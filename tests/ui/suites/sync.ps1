# Syncing YouTube Music playlists: Sync all, Sync selected and Sync now fetch them into state.json and the track table,
# Spotify playlists are left out, a failed fetch keeps its error, Cancel stops a fetch, removing a playlist takes its
# tracks along, and a fetch waits for yt-dlp to be installed. Dot-sourced by ui-checks.ps1.

# A state.json with these YouTube Music and Spotify playlists, never synced, added a day apart in this order
function Write-SyncPlaylists([string[]]$keys) {
	$day = 0
	$entries = $keys | ForEach-Object {
		$day++
		$id = $_.Substring($_.IndexOf(':') + 1)
		$url = if ($_ -like 'spotify:*') { "https://open.spotify.com/playlist/$id" } else { "https://music.youtube.com/playlist?list=$id" }
		"`"$_`": {`"url`": `"$url`", `"name`": `"Named $id`", `"addedAt`": `"2026-01-$('{0:D2}' -f $day)T12:00:00Z`"}"
	}
	[IO.File]::WriteAllText($StateFile, '{"version": 1, "playlists": {' + ($entries -join ', ') + '}}')
}

function Get-SavedState { Get-Content $StateFile -Raw -Encoding UTF8 | ConvertFrom-Json }

# Each playlist as "key [name] its sources (without ytm:track) synced: whether lastSync is a time", and its error
function Get-SavedPlaylistSync {
	@((Get-SavedState).playlists.PSObject.Properties | ForEach-Object {
		$p = $_.Value
		$sources = @($p.sources | ForEach-Object { $_ -replace '^ytm:track', '' }) -join ','
		$failure = if ($p.fetchError) { " error: $($p.fetchError) at $($p.fetchErrorAt -match '^\d{4}-\d\d-\d\dT')" } else { '' }
		"$($_.Name) [$($p.name)] $sources synced $($p.lastSync -match '^\d{4}-\d\d-\d\dT')$failure"
	}) -join ' / '
}

# Each source as "id title [artists] milliseconds > its track"
function Get-SavedSources {
	@((Get-SavedState).sources.PSObject.Properties | ForEach-Object {
		$s = $_.Value
		"$($_.Name -replace '^ytm:track', '') $($s.title) [$($s.artists -join ', ')] $($s.durationMs) > $($s.track -replace '^ytm:track', '')"
	}) -join ' / '
}

# Each track as "id status [its sources] whether addedAt is a time"
function Get-SavedTracks {
	@((Get-SavedState).tracks.PSObject.Properties | ForEach-Object {
		$t = $_.Value
		"$($_.Name -replace '^ytm:track', '') $($t.status) [$(@($t.sources | ForEach-Object { $_ -replace '^ytm:track', '' }) -join ',')] $($t.addedAt -match '^\d{4}-\d\d-\d\dT')"
	}) -join ' / '
}

function Get-FetchRuns {
	if (-not (Test-Path $FakeYtDlpRuns)) { return 0 }
	@(Get-Content $FakeYtDlpRuns -Encoding UTF8 | Where-Object { $_ -notlike '*--playlist-items*' }).Count
}

# A sync enables Cancel until it ends
function Test-Syncing($hwnd) {
	Update-MenuStates $hwnd
	$menu = [W]::GetMenu($hwnd)
	([W]::GetMenuState($menu, [W]::CommandOf($menu, '&Cancel'), 0) -band 0x1) -eq 0
}

# Waits until $count fetches more than $before have started, and then for the sync to end. yt-dlp logs a fetch only
# after the sync was queued, so by then the window knows a sync is going.
function Wait-Fetches($hwnd, [int]$before, [int]$count) {
	for ($i = 0; $i -lt 150 -and (Get-FetchRuns) -lt $before + $count; $i++) { Start-Sleep -Milliseconds 100 }
	for ($i = 0; $i -lt 150 -and (Test-Syncing $hwnd); $i++) { Start-Sleep -Milliseconds 100 }
	Start-Sleep -Milliseconds 200
}

# The table's rows, each as its cells in display order
function Get-TrackRows($hwnd) {
	$list = Get-TrackTable $hwnd
	$count = [int][W]::SendMessageW($list, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero)
	@(if ($count -gt 0) { 0..($count - 1) | ForEach-Object { [W]::ListViewRow($list, $_) } }) -join ' / '
}

# Selects row $index (from 0) of the table with Home and Down. A sent click would do, but the list sees the real cursor
# away from it and starts a drag, which then takes the next click on the header.
function Select-TrackRow($hwnd, [int]$index) {
	$list = Get-TrackTable $hwnd
	[void][W]::SendMessageW($list, 0x100, [IntPtr]0x24, [IntPtr]1)
	for ($i = 0; $i -lt $index; $i++) { [void][W]::SendMessageW($list, 0x100, [IntPtr]0x28, [IntPtr]1) }
}

# The selected rows in All tracks, each as its index and title
function Get-SelectedRows($hwnd) {
	$list = Get-TrackTable $hwnd
	$rows = @()
	$item = -1
	while (($item = [int][W]::SendMessageW($list, 0x100C, [IntPtr]$item, [IntPtr]2)) -ne -1) {
		$rows += "$item $(([W]::ListViewRow($list, $item) -split ' \| ')[1])"
	}
	$rows -join ' / '
}

function Get-TrackSummary($hwnd) { "$([W]::TabTexts((Get-StatusTabs $hwnd))) | $([W]::TextOf((Get-ResultLine $hwnd)))" }

$Cafe = "Caf$([char]0xE9) Lights"
$TracksName = 'Checked PLtracks1'
$SharedName = 'Checked PLshared1'
$SlowName = 'Checked PLslow1'
$BrokenError = 'Unable to download API page: HTTP Error 503: Service Unavailable'
$SyncSpotifyKey = "spotify:$SpotifyId"
$RowA = " | Way Down We Go | KALEO |  | $TracksName | 3:34"
$RowB = " | $Cafe | Nova |  | $TracksName | 3:11"
$RowC = " | No Length | Somebody |  | $TracksName | 0:00"
$RowD = " | Only Here | Solo |  | $SharedName | 1:05"
$RowShared = " | Shared Song | Both |  | $TracksName, $SharedName | 3:20"
$RowSharedAlone = " | Shared Song | Both |  | $TracksName | 3:20"
$RowSlow = " | Slow One | Turtle |  | $SlowName | 5:00"

"--- Run 1: syncing"
Set-SavedWindow @{}
Install-FakeTools
Set-SavedAutoUpdate $false
Write-SyncPlaylists 'ytm:PLtracks1', $SyncSpotifyKey, 'ytm:PLshared1', 'ytm:PLbroken1'
$app = Start-App
$h = $app.Hwnd
Check 'Before a sync: tabs | result line' (Get-TrackSummary $h) 'All 0 | Synced 0 | Waiting 0 | Needs review 0 | Failed 0 | Showing 0 of 0'

$runs = Get-FetchRuns
Send-Command $h 'Sync &all'
Wait-Fetches $h $runs 3
Check 'Sync all: YouTube Music playlists fetched, Spotify left out' (Get-FetchRuns) ($runs + 3)
Check '  each fetch reads the whole playlist' (@(Get-Content $FakeYtDlpRuns -Encoding UTF8)[-1]) '--ignore-config|--flat-playlist|--dump-single-json|https://music.youtube.com/playlist?list=PLbroken1'
Check '  playlists' (Get-SavedPlaylistSync) ("$SyncSpotifyKey [Named $SpotifyId]  synced False / ytm:PLbroken1 [Named PLbroken1]  synced False error: $BrokenError at True / " +
	"ytm:PLshared1 [$SharedName] SHARE5,DDDDD6 synced True / ytm:PLtracks1 [$TracksName] AAAAA1,BBBBB2,CCCCC4,SHARE5,AAAAA1 synced True")
Check '  sources: Topic dropped, a private video left out' (Get-SavedSources) ("AAAAA1 Way Down We Go [KALEO] 214000 > AAAAA1 / BBBBB2 $Cafe [Nova] 191000 > BBBBB2 / " +
	'CCCCC4 No Length [Somebody] 0 > CCCCC4 / DDDDD6 Only Here [Solo] 65000 > DDDDD6 / SHARE5 Shared Song [Both] 200000 > SHARE5')
Check '  tracks' (Get-SavedTracks) 'AAAAA1 pending [AAAAA1] True / BBBBB2 pending [BBBBB2] True / CCCCC4 pending [CCCCC4] True / DDDDD6 pending [DDDDD6] True / SHARE5 pending [SHARE5] True'
Check '  All tracks: rows, by title' (Get-TrackRows $h) "$RowB / $RowC / $RowD / $RowShared / $RowA"
Check '  tabs | result line' (Get-TrackSummary $h) 'All 5 | Synced 0 | Waiting 5 | Needs review 0 | Failed 0 | Showing 5 of 5'
Check '  Library card' (Get-CardTexts $h) "Library card: All tracks / Library / Folder / $DefaultLibrary / Tracks / 5 tracks $MiddleDot 0 synced / Open library folder"
Check '  syncing after it' (Test-Syncing $h) 'False'

Click-PlaylistRow $h 1
Check 'Playlist 1: rows by position, a repeat once' (Get-TrackRows $h) "1 | $RowA / 2 | $RowB / 3 | $RowC / 4 | $RowShared"
Check '  tabs | result line' (Get-TrackSummary $h) "All 4 | Synced 0 | Waiting 4 | Needs review 0 | Failed 0 | Showing 4 of 5 in $TracksName"
Check '  card' (Get-CardTexts $h) ("Playlist card: $TracksName / YouTube Music playlist / Playlist / Link / PLtracks1 / Tracks / 4 / Synced / 0/4 / " +
	"Last sync / Never / Sync playlist / Remove playlist$Ellipsis")
Click-PlaylistRow $h 2
Check 'The Spotify playlist: no rows' "$(Get-TrackRows $h)| $(Get-TrackSummary $h)" "| All 0 | Synced 0 | Waiting 0 | Needs review 0 | Failed 0 | Showing 0 of 5 in Named $SpotifyId"
$runs = Get-FetchRuns
Send-Command $h 'Sync &selected playlist'
Start-Sleep -Milliseconds 1000
Check '  Sync selected: nothing fetched' "$(Get-FetchRuns) $(Test-Syncing $h)" "$runs False"

# Sync now syncs the playlist that was just added; Cancel stops its fetch, which keeps the playlist as it was
$runs = Get-FetchRuns
Check 'Add a playlist with Sync now' (Add-Playlist $h 'https://music.youtube.com/playlist?list=PLslow1' -SyncNow) "$SlowName YouTube Music playlist | Add True"
for ($i = 0; $i -lt 50 -and (Get-FetchRuns) -eq $runs; $i++) { Start-Sleep -Milliseconds 100 }
Check '  its fetch starts | syncing | Sync all' "$((Get-FetchRuns) - $runs) $(Test-Syncing $h) $(([W]::GetMenuState([W]::GetMenu($h), [W]::CommandOf([W]::GetMenu($h), 'Sync &all'), 0) -band 0x1) -ne 0)" '1 True True'
$started = [DateTime]::Now
Send-Command $h '&Cancel'
for ($i = 0; $i -lt 50 -and (Test-Syncing $h); $i++) { Start-Sleep -Milliseconds 100 }
$took = ([DateTime]::Now - $started).TotalMilliseconds
Check-That '  Cancel ends it before yt-dlp answers' "$([int]$took) ms" ($took -lt 2000) 'under 2000 ms'
Check '  the playlist stays unsynced' (@((Get-SavedPlaylistSync) -split ' / ' | Where-Object { $_ -like 'ytm:PLslow1*' })) "ytm:PLslow1 [$SlowName]  synced False"
Check '  rows' (Get-TrackRows $h) ''

$runs = Get-FetchRuns
Send-Command $h 'Sync &selected playlist'
Wait-Fetches $h $runs 1
Check 'Sync selected: its rows' (Get-TrackRows $h) "1 | $RowSlow"
Check '  tabs | result line' (Get-TrackSummary $h) "All 1 | Synced 0 | Waiting 1 | Needs review 0 | Failed 0 | Showing 1 of 6 in $SlowName"
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-sync.png") }

# Removing a playlist deletes the tracks it alone listed, which have no file yet
Click-PlaylistRow $h 3
Send-Command $h '&Remove playlist...'
$dialog = Wait-OwnedWindow $h
[void][W]::PostMessageW($dialog, 0x466, [IntPtr]1, [IntPtr]::Zero)
Wait-Closed $dialog
Check 'Remove the second playlist: tracks' (Get-SavedTracks) 'AAAAA1 pending [AAAAA1] True / BBBBB2 pending [BBBBB2] True / CCCCC4 pending [CCCCC4] True / SHARE5 pending [SHARE5] True / SLOWW7 pending [SLOWW7] True'
Check '  sources' (@((Get-SavedState).sources.PSObject.Properties | ForEach-Object { $_.Name -replace '^ytm:track', '' }) -join ' ') 'AAAAA1 BBBBB2 CCCCC4 SHARE5 SLOWW7'
Check '  All tracks: rows' (Get-TrackRows $h) "$RowB / $RowC / $RowSharedAlone / $RowSlow / $RowA"
Check '  tabs | result line' (Get-TrackSummary $h) 'All 5 | Synced 0 | Waiting 5 | Needs review 0 | Failed 0 | Showing 5 of 5'
Stop-App $app

"--- Run 2: tracks kept"
$app = Start-App
$h = $app.Hwnd
Check 'Rows after a restart' (Get-TrackRows $h) "$RowB / $RowC / $RowSharedAlone / $RowSlow / $RowA"
Check '  tabs | result line' (Get-TrackSummary $h) 'All 5 | Synced 0 | Waiting 5 | Needs review 0 | Failed 0 | Showing 5 of 5'

# A header click sorts the rows by its column, a second one turns them round; rows that tie go by title
Click-Header $h 5
Check 'Sorted by Time' (Get-TrackRows $h) "$RowC / $RowB / $RowSharedAlone / $RowA / $RowSlow"
Click-Header $h 5
Check '  again: longest first' (Get-TrackRows $h) "$RowSlow / $RowA / $RowSharedAlone / $RowB / $RowC"
Click-Header $h 2
Check 'Sorted by Artists' (Get-TrackRows $h) "$RowSharedAlone / $RowA / $RowB / $RowC / $RowSlow"
Click-Header $h 0
Check 'Sorted by Status: all wait, so by title' (Get-TrackRows $h) "$RowB / $RowC / $RowSharedAlone / $RowSlow / $RowA"

# The selection stays with its track when the rows move or narrow
Select-TrackRow $h 3
Check 'Select row 4' (Get-SelectedRows $h) '3 Slow One'
Click-Header $h 5
Check '  sorted by Time: it moves along' (Get-SelectedRows $h) '4 Slow One'
Choose-Source $h 1
Check 'Source YouTube Music: tabs | result line' (Get-TrackSummary $h) 'All 5 | Synced 0 | Waiting 5 | Needs review 0 | Failed 0 | Showing 5 of 5'
Check '  selection kept' (Get-SelectedRows $h) '4 Slow One'
Choose-Source $h 2
Check 'Source Spotify: rows | tabs | result line' "$(Get-TrackRows $h)| $(Get-TrackSummary $h)" '| All 0 | Synced 0 | Waiting 0 | Needs review 0 | Failed 0 | Showing 0 of 5'
Choose-Source $h 0
Check '  back to All: the track filtered out stays unselected' "[$(Get-SelectedRows $h)]" '[]'

# Another tab or scope starts with nothing selected
Select-TrackRow $h 1
Click-Tab $h 1
Check 'Synced tab: rows | tabs | result line' "$(Get-TrackRows $h)| $(Get-TrackSummary $h)" '| All 5 | Synced 0 | Waiting 5 | Needs review 0 | Failed 0 | Showing 0 of 5'
Click-Tab $h 2
Check 'Waiting tab: rows' (Get-TrackRows $h) "$RowC / $RowB / $RowSharedAlone / $RowA / $RowSlow"
Check '  the row selected under All is not' "[$(Get-SelectedRows $h)]" '[]'
Select-TrackRow $h 1
Click-Tab $h 0
Check 'All tab: selection cleared' "[$(Get-SelectedRows $h)]" '[]'
Select-TrackRow $h 1
Click-PlaylistRow $h 1
Check 'Playlist 1: selection cleared' "[$(Get-SelectedRows $h)]" '[]'
Select-TrackRow $h 1
Click-AllTracks $h
Check 'All tracks: selection cleared' "[$(Get-SelectedRows $h)]" '[]'

Click-PlaylistRow $h 1
Click-Header $h 0
Check 'Playlist 1 by #, reversed' (Get-TrackRows $h) "4 | $RowSharedAlone / 3 | $RowC / 2 | $RowB / 1 | $RowA"
Click-Header $h 2
Check '  by title, # kept' (Get-TrackRows $h) "2 | $RowB / 3 | $RowC / 4 | $RowSharedAlone / 1 | $RowA"
Stop-App $app

"--- Run 3: a fetch waits for yt-dlp"
Remove-Item $ToolsDir -Recurse -Force
Write-SyncPlaylists 'ytm:PLtracks1'
Remove-Item "$StateFile.bak" -ErrorAction SilentlyContinue
$Web.YtDlp = [IO.File]::ReadAllBytes($FakeYtDlp)
$env:WESTIELIB_GITHUB_API = $Web.Url
$app = Start-App -DuringToolRun
$h = $app.Hwnd
Send-Command $h 'Sync &all'
Start-Sleep -Milliseconds 1000
Check 'yt-dlp downloading: syncing | nothing fetched' "$(Test-Syncing $h) $(Get-FetchRuns) | $(Get-SavedPlaylistSync)" 'True 0 | ytm:PLtracks1 [Named PLtracks1]  synced False'
[void]$Web.AssetGate.Set()
Wait-Fetches $h 0 1
Check '  installed: the fetch goes ahead' (Get-SavedPlaylistSync) "ytm:PLtracks1 [$TracksName] AAAAA1,BBBBB2,CCCCC4,SHARE5,AAAAA1 synced True"
Check '  rows' (Get-TrackRows $h) "$RowB / $RowC / $RowSharedAlone / $RowA"
Wait-ToolsIdle $h
Stop-App $app
