# Syncing playlists: Sync all, Sync selected and Sync now fetch YouTube Music (from the fake yt-dlp) and Spotify (from
# FakeWeb) playlists into state.json and the track table, a failed fetch keeps its error, Cancel stops a fetch, removing
# a playlist takes its tracks along, a fetch waits for yt-dlp to be installed, and a Spotify fetch shows each page as it
# comes and fails as Spotify does. Dot-sourced by ui-checks.ps1.

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

# Source and track keys without their ytm:track or spotify: prefix
$KeyPrefix = '^(ytm:track|spotify:)'

# Each playlist as "key [name] its sources (without prefix) synced: whether lastSync is a time", and its error
function Get-SavedPlaylistSync {
	@((Get-SavedState).playlists.PSObject.Properties | ForEach-Object {
		$p = $_.Value
		$sources = @($p.sources | ForEach-Object { $_ -replace $KeyPrefix, '' }) -join ','
		$failure = if ($p.fetchError) { " error: $($p.fetchError) at $($p.fetchErrorAt -match '^\d{4}-\d\d-\d\dT')" } else { '' }
		"$($_.Name) [$($p.name)] $sources synced $($p.lastSync -match '^\d{4}-\d\d-\d\dT')$failure"
	}) -join ' / '
}

# Each source as "id title [artists] (album, when it has the key) milliseconds > its track"
function Get-SavedSources {
	@((Get-SavedState).sources.PSObject.Properties | ForEach-Object {
		$s = $_.Value
		$album = if ($null -ne $s.album) { " ($($s.album))" } else { '' }
		"$($_.Name -replace $KeyPrefix, '') $($s.title) [$($s.artists -join ', ')]$album $($s.durationMs) > $($s.track -replace $KeyPrefix, '')"
	}) -join ' / '
}

# Each track as "id status [its sources] whether addedAt is a time"
function Get-SavedTracks {
	@((Get-SavedState).tracks.PSObject.Properties | ForEach-Object {
		$t = $_.Value
		"$($_.Name -replace $KeyPrefix, '') $($t.status) [$(@($t.sources | ForEach-Object { $_ -replace $KeyPrefix, '' }) -join ',')] $($t.addedAt -match '^\d{4}-\d\d-\d\dT')"
	}) -join ' / '
}

function Get-FetchRuns {
	if (-not (Test-Path $FakeYtDlpRuns)) { return 0 }
	@(Get-Content $FakeYtDlpRuns -Encoding UTF8 | Where-Object { $_ -notlike '*--playlist-items*' }).Count
}

function Get-SpotifyQueries { @($Web.SpotifyQueries()).Count }

# Waits until FakeWeb has had $count Spotify queries; the app shows a page before it asks for the next
function Wait-SpotifyQueries([int]$count) {
	for ($i = 0; $i -lt 150 -and (Get-SpotifyQueries) -lt $count; $i++) { Start-Sleep -Milliseconds 100 }
	Start-Sleep -Milliseconds 300
}

# The big Spotify playlist as "name, source count, its first and 101st source, synced: whether lastSync is a time, its error"
function Get-BigPlaylist {
	$big = (Get-SavedState).playlists."spotify:$BigSpotifyId"
	$sources = @($big.sources | ForEach-Object { $_ -replace $KeyPrefix, '' })
	$hundredFirst = if ($sources.Count -gt 100) { $sources[100] } else { '-' }
	"$($big.name) $($sources.Count) $($sources[0]) $hundredFirst synced $($big.lastSync -match '^\d{4}-\d\d-\d\dT') error [$($big.fetchError)]"
}

function New-BigTrack([int]$n) { [FakeWeb]::Track("spBigTrack$('{0:D12}' -f $n)", "Big Song $('{0:D3}' -f $n)", @('Many'), 'Big', 60000) }

# A sync enables Cancel until it ends
function Test-Syncing($hwnd) {
	Update-MenuStates $hwnd
	$menu = [W]::GetMenu($hwnd)
	([W]::GetMenuState($menu, [W]::CommandOf($menu, '&Cancel'), 0) -band 0x1) -eq 0
}

# Waits until $count fetches more than $before have started, and then for the sync to end. yt-dlp logs a fetch only
# after the sync was queued, so by then the window knows a sync is going. With -Spotify, it counts FakeWeb's Spotify
# queries instead, which come after the sync was queued too.
function Wait-Fetches($hwnd, [int]$before, [int]$count, [switch]$Spotify) {
	$counter = if ($Spotify) { { Get-SpotifyQueries } } else { { Get-FetchRuns } }
	for ($i = 0; $i -lt 150 -and (& $counter) -lt $before + $count; $i++) { Start-Sleep -Milliseconds 100 }
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

# Whether the infobar shows, and its message
function Get-InfobarText($hwnd) {
	$infobar = Get-Infobar $hwnd
	"$([W]::IsWindowVisible($infobar)) $([W]::TextOf(([W]::Children($infobar, 'WestieLib.Label'))[0]))"
}

# The Add playlist button follows the rows, however tall each is
function Check-ListButton([string]$what, $hwnd, [int]$count) {
	$list = Get-PlaylistList $hwnd
	$button = [W]::RectIn(([W]::Children($list, 'Button'))[0], $list)
	Check $what $button.Top ((Get-PlaylistRowTop $hwnd ($count + 1)) + (S 8))
}

function Get-TrackSummary($hwnd) { "$([W]::TabTexts((Get-StatusTabs $hwnd))) | $([W]::TextOf((Get-ResultLine $hwnd)))" }

$Cafe = "Caf$([char]0xE9) Lights"
$TracksName = 'Checked PLtracks1'
$SharedName = 'Checked PLshared1'
$SlowName = 'Checked PLslow1'
$BrokenError = 'Unable to download API page: HTTP Error 503: Service Unavailable'
$SyncSpotifyKey = "spotify:$SpotifyId"
$FetchMessage = "Couldn$([char]0x2019)t fetch $([char]0x201C)Named PLbroken1$([char]0x201D). It may be private or deleted."
$RowA = " | Way Down We Go | KALEO |  | $TracksName | 3:34"
$RowB = " | $Cafe | Nova |  | $TracksName | 3:11"
$RowC = " | No Length | Somebody |  | $TracksName | 0:00"
$RowD = " | Only Here | Solo |  | $SharedName | 1:05"
$RowShared = " | Shared Song | Both |  | $TracksName, $SharedName | 3:20"
$RowSharedAlone = " | Shared Song | Both |  | $TracksName | 3:20"
$RowSlow = " | Slow One | Turtle |  | $SlowName | 5:00"
$Senorita = "Se$([char]0xF1)orita"
$RowLate = " | Late Night Swing | Ana, Ben | Nights | $SpotifyName | 4:01"
$RowSenorita = " | $Senorita | Earth, Wind & Fire | Summer | $SpotifyName | 3:05"
$SpotifyQuery = "$SpotifyId 0 100 Bearer fake-token-$SpotifyId WebPlayer"

# The Spotify playlist: two tracks, one by a single artist with a comma in its name, an episode and a local file (both
# left out), and a repeat
$late = [FakeWeb]::Track('spTrackLateNight000001', 'Late Night Swing', @('Ana', 'Ben'), 'Nights', 241000)
$Web.SpotifyItems[$SpotifyId] = [Collections.Generic.List[string]]@(
	$late,
	[FakeWeb]::Episode('spEpisode0000000000003', 'An Episode'),
	[FakeWeb]::Track('spTrackSenorita0000002', $Senorita, @('Earth, Wind & Fire'), 'Summer', 185000),
	[FakeWeb]::LocalFile('My Own Song'),
	$late
)
# One over a page, one whose query Spotify refuses, and $MissingSpotifyId, whose embed page is Spotify's error page
$BigSpotifyId = '1bigbigbigbigbigbigbig'
$ErrorSpotifyId = '2errerrerrerrerrerrerr'
$Web.SpotifyNames[$BigSpotifyId] = 'Big One'
$Web.SpotifyNames[$ErrorSpotifyId] = 'Refused'
$Web.SpotifyItems[$BigSpotifyId] = [Collections.Generic.List[string]]@(1..150 | ForEach-Object { New-BigTrack $_ })
[void]$Web.ErrorQueryIds.Add($ErrorSpotifyId)

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
Check 'Sync all: YouTube Music playlists fetched by yt-dlp' (Get-FetchRuns) ($runs + 3)
Check '  each fetch reads the whole playlist' (@(Get-Content $FakeYtDlpRuns -Encoding UTF8)[-1]) '--ignore-config|--flat-playlist|--dump-single-json|https://music.youtube.com/playlist?list=PLbroken1'
Check '  the Spotify playlist: one query, with the page''s token' ($Web.SpotifyQueries() -join ' / ') $SpotifyQuery
Check '  playlists' (Get-SavedPlaylistSync) ("$SyncSpotifyKey [$SpotifyName] spTrackLateNight000001,spTrackSenorita0000002,spTrackLateNight000001 synced True / " +
	"ytm:PLbroken1 [Named PLbroken1]  synced False error: $BrokenError at True / " +
	"ytm:PLshared1 [$SharedName] SHARE5,DDDDD6 synced True / ytm:PLtracks1 [$TracksName] AAAAA1,BBBBB2,CCCCC4,SHARE5,AAAAA1 synced True")
Check '  sources: Topic dropped, a private video, an episode and a local file left out, albums from Spotify only' (Get-SavedSources) (
	'spTrackLateNight000001 Late Night Swing [Ana, Ben] (Nights) 241000 > spTrackLateNight000001 / ' +
	"spTrackSenorita0000002 $Senorita [Earth, Wind & Fire] (Summer) 185000 > spTrackSenorita0000002 / " +
	"AAAAA1 Way Down We Go [KALEO] 214000 > AAAAA1 / BBBBB2 $Cafe [Nova] 191000 > BBBBB2 / " +
	'CCCCC4 No Length [Somebody] 0 > CCCCC4 / DDDDD6 Only Here [Solo] 65000 > DDDDD6 / SHARE5 Shared Song [Both] 200000 > SHARE5')
Check '  tracks' (Get-SavedTracks) ('spTrackLateNight000001 pending [spTrackLateNight000001] True / spTrackSenorita0000002 pending [spTrackSenorita0000002] True / ' +
	'AAAAA1 pending [AAAAA1] True / BBBBB2 pending [BBBBB2] True / CCCCC4 pending [CCCCC4] True / DDDDD6 pending [DDDDD6] True / SHARE5 pending [SHARE5] True')
Check '  All tracks: rows, by title' (Get-TrackRows $h) "$RowB / $RowLate / $RowC / $RowD / $RowSenorita / $RowShared / $RowA"
Check '  tabs | result line' (Get-TrackSummary $h) 'All 7 | Synced 0 | Waiting 7 | Needs review 0 | Failed 0 | Showing 7 of 7'
Check '  Library card' (Get-CardTexts $h) "Library card: All tracks / Library / Folder / $DefaultLibrary / Tracks / 7 tracks $MiddleDot 0 synced / Open library folder"
Check '  syncing after it' (Test-Syncing $h) 'False'
# The rows with tracks waiting have a line for them
$script:TallRows = 1, 2, 3
Check-ListButton '  rows 1 to 3 a line taller: Add playlist under them' $h 4
Check '  infobar: the failed fetch' (Get-InfobarText $h) "True $FetchMessage"
Check '  the rows sync controls tooltips' "$([W]::TooltipText((Get-PlaylistList $h), 0)) / $([W]::TooltipText((Get-PlaylistList $h), 3))" "Sync $TracksName / Sync Named PLbroken1"

# One selected track shows its card; a link to one of its playlists selects that playlist
Select-TrackRow $h 5
Check 'Shared Song selected: its card' (Wait-CardTexts $h 'Track card') ("Track card:  / Shared Song / Both / Album is read when it downloads / " +
	"Status / Waiting / Downloads on the next sync. / Source / Source / YouTube Music $MiddleDot Open / Playlists / $TracksName, $SharedName / " +
	"Key / ytm:trackSHARE5 / Audio / Video / Chosen when it downloads / File / Not downloaded yet.")
Select-TrackRow $h 1
Check 'Late Night Swing selected: its card' (Wait-CardTexts $h 'Track card') ("Track card:  / Late Night Swing / Ana, Ben / Nights / " +
	"Status / Waiting / Downloads on the next sync. / Source / Source / Spotify $MiddleDot Open track / Playlists / $SpotifyName / " +
	"Key / spotify:spTrackLateNight000001 / Audio / Video / Chosen when it downloads / File / Not downloaded yet.")
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-track.png") }
$card = Get-InspectorCard $h
$cover =[W]::RectIn(([W]::Children($card, 'WestieLib.CoverArt'))[0], $card)
Check '  cover: 144 square, centered' "$($cover.Right - $cover.Left)x$($cover.Bottom - $cover.Top) $($cover.Left - ((Get-ClientWidth $card) - $cover.Right))" "$(S 144)x$(S 144) 0"
# Only the card's own link: the source rows' links open a browser
$playlistLinks = @(Get-DirectChildren $card 'WestieLib.LinkLabel')[0]
if ([W]::TextOf($playlistLinks) -eq $SpotifyName) { Send-Click $playlistLinks 2 (S 8) }
Check '  its playlist link: the playlist selected | card' "$(Get-Selection $h 4) | $([W]::TextOf((Get-InspectorCard $h)))" 'row 2 | Playlist card'
Check '  no track selected' "[$(Get-SelectedRows $h)]" '[]'
Click-AllTracks $h

Click-PlaylistRow $h 1
Check 'Playlist 1: rows by position, a repeat once' (Get-TrackRows $h) "1 | $RowA / 2 | $RowB / 3 | $RowC / 4 | $RowShared"
Check '  tabs | result line' (Get-TrackSummary $h) "All 4 | Synced 0 | Waiting 4 | Needs review 0 | Failed 0 | Showing 4 of 7 in $TracksName"
Check '  card' ((Get-CardTexts $h) -replace 'today \d\d:\d\d', 'today HH:MM') ("Playlist card: $TracksName / YouTube Music playlist / Playlist / Link / PLtracks1 / " +
	"Tracks / 4 / Synced / 0/4 / Waiting / 4 / Shared / 1 track is also in other playlists / Last sync / today HH:MM / Sync playlist / Remove playlist$Ellipsis")
Click-PlaylistRow $h 2
Check 'The Spotify playlist: rows by position, a repeat once' (Get-TrackRows $h) "1 | $RowLate / 2 | $RowSenorita"
Check '  tabs | result line' (Get-TrackSummary $h) "All 2 | Synced 0 | Waiting 2 | Needs review 0 | Failed 0 | Showing 2 of 7 in $SpotifyName"
$runs = Get-FetchRuns
$queries = Get-SpotifyQueries
Send-Command $h 'Sync &selected playlist'
Wait-Fetches $h $queries 1 -Spotify
Check '  Sync selected: one query, no yt-dlp | rows' "$((Get-SpotifyQueries) - $queries) $((Get-FetchRuns) - $runs) | $(Get-TrackRows $h)" "1 0 | 1 | $RowLate / 2 | $RowSenorita"

# Details shows the log; the infobar's close box dismisses the failure until the playlist fails again
$infobar = Get-Infobar $h
[void][W]::PostMessageW(([W]::Children($infobar, 'Button'))[0], 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Start-Sleep -Milliseconds 300
Check 'Infobar Details: the log shows' (Get-LogVisibility $h) 'True True'
Send-Command $h '&Log'
Start-Sleep -Milliseconds 200
Send-Click $infobar ((Get-ClientWidth $infobar) - (S 6) - (S 12)) ([int](((Get-ClientHeight $infobar) - 1) / 2))
Start-Sleep -Milliseconds 300
Check '  its close box: gone, the panels move up' "$([W]::IsWindowVisible($infobar)) $((Get-RowPanels $h).Count)" 'False 3'
$runs = Get-FetchRuns
Click-PlaylistRow $h 4
Send-Command $h 'Sync &selected playlist'
Wait-Fetches $h $runs 1
Check '  the playlist fails again: back' (Get-InfobarText $h) "True $FetchMessage"

# A row's sync control syncs its playlist
$list = Get-PlaylistList $h
$control = [W]::ToolRect($list, 0)
$runs = Get-FetchRuns
Send-Click $list ([int](($control.Left + $control.Right) / 2)) ([int](($control.Top + $control.Bottom) / 2))
Wait-Fetches $h $runs 1
Check 'Row 1 sync control: one fetch, its playlist' "$((Get-FetchRuns) - $runs) $(@(Get-Content $FakeYtDlpRuns -Encoding UTF8)[-1] -replace '^.*list=', '')" '1 PLtracks1'

# Sync now syncs the playlist that was just added; Cancel stops its fetch, which keeps the playlist as it was
$runs = Get-FetchRuns
Check 'Add a playlist with Sync now' (Add-Playlist $h 'https://music.youtube.com/playlist?list=PLslow1' -SyncNow) "$SlowName YouTube Music playlist | Add True"
for ($i = 0; $i -lt 50 -and (Get-FetchRuns) -eq $runs; $i++) { Start-Sleep -Milliseconds 100 }
Check '  its fetch starts | syncing | Sync all' "$((Get-FetchRuns) - $runs) $(Test-Syncing $h) $(([W]::GetMenuState([W]::GetMenu($h), [W]::CommandOf([W]::GetMenu($h), 'Sync &all'), 0) -band 0x1) -ne 0)" '1 True True'
Check '  while it syncs, the controls have no tooltip' "[$([W]::TooltipText((Get-PlaylistList $h), 0))]" '[]'
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-syncing.png") }
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
$script:TallRows = 1, 2, 3, 5
Check 'Sync selected: its rows' (Get-TrackRows $h) "1 | $RowSlow"
Check '  tabs | result line' (Get-TrackSummary $h) "All 1 | Synced 0 | Waiting 1 | Needs review 0 | Failed 0 | Showing 1 of 8 in $SlowName"
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-sync.png") }

# Removing a playlist deletes the tracks it alone listed, which have no file yet
Click-PlaylistRow $h 3
Send-Command $h '&Remove playlist...'
$dialog = Wait-OwnedWindow $h
[void][W]::PostMessageW($dialog, 0x466, [IntPtr]1, [IntPtr]::Zero)
Wait-Closed $dialog
$script:TallRows = 1, 2, 4
Check 'Remove the third playlist: tracks' (Get-SavedTracks) ('spTrackLateNight000001 pending [spTrackLateNight000001] True / spTrackSenorita0000002 pending [spTrackSenorita0000002] True / ' +
	'AAAAA1 pending [AAAAA1] True / BBBBB2 pending [BBBBB2] True / CCCCC4 pending [CCCCC4] True / SHARE5 pending [SHARE5] True / SLOWW7 pending [SLOWW7] True')
Check '  sources' (@((Get-SavedState).sources.PSObject.Properties | ForEach-Object { $_.Name -replace $KeyPrefix, '' }) -join ' ') (
	'spTrackLateNight000001 spTrackSenorita0000002 AAAAA1 BBBBB2 CCCCC4 SHARE5 SLOWW7')
Check '  All tracks: rows' (Get-TrackRows $h) "$RowB / $RowLate / $RowC / $RowSenorita / $RowSharedAlone / $RowSlow / $RowA"
Check '  tabs | result line' (Get-TrackSummary $h) 'All 7 | Synced 0 | Waiting 7 | Needs review 0 | Failed 0 | Showing 7 of 7'
Stop-App $app

"--- Run 2: tracks kept"
$app = Start-App
$h = $app.Hwnd
Check 'Rows after a restart' (Get-TrackRows $h) "$RowB / $RowLate / $RowC / $RowSenorita / $RowSharedAlone / $RowSlow / $RowA"
Check '  tabs | result line' (Get-TrackSummary $h) 'All 7 | Synced 0 | Waiting 7 | Needs review 0 | Failed 0 | Showing 7 of 7'

# A header click sorts the rows by its column, a second one turns them round; rows that tie go by title
Click-Header $h 5
Check 'Sorted by Time' (Get-TrackRows $h) "$RowC / $RowSenorita / $RowB / $RowSharedAlone / $RowA / $RowLate / $RowSlow"
Click-Header $h 5
Check '  again: longest first' (Get-TrackRows $h) "$RowSlow / $RowLate / $RowA / $RowSharedAlone / $RowB / $RowSenorita / $RowC"
Click-Header $h 2
Check 'Sorted by Artists' (Get-TrackRows $h) "$RowLate / $RowSharedAlone / $RowSenorita / $RowA / $RowB / $RowC / $RowSlow"
Click-Header $h 3
Check 'Sorted by Album: the YouTube Music rows have none' (Get-TrackRows $h) "$RowB / $RowC / $RowSharedAlone / $RowSlow / $RowA / $RowLate / $RowSenorita"
Click-Header $h 0
Check 'Sorted by Status: all wait, so by title' (Get-TrackRows $h) "$RowB / $RowLate / $RowC / $RowSenorita / $RowSharedAlone / $RowSlow / $RowA"

# The selection stays with its track when the rows move or narrow
Select-TrackRow $h 5
Check 'Select row 6' (Get-SelectedRows $h) '5 Slow One'
Click-Header $h 5
Check '  sorted by Time: it moves along' (Get-SelectedRows $h) '6 Slow One'
Choose-Source $h 1
Check 'Source YouTube Music: tabs | result line' (Get-TrackSummary $h) 'All 5 | Synced 0 | Waiting 5 | Needs review 0 | Failed 0 | Showing 5 of 7'
Check '  selection kept' (Get-SelectedRows $h) '4 Slow One'
Choose-Source $h 2
Check 'Source Spotify: rows | tabs | result line' "$(Get-TrackRows $h) | $(Get-TrackSummary $h)" (
	"$RowSenorita / $RowLate | All 2 | Synced 0 | Waiting 2 | Needs review 0 | Failed 0 | Showing 2 of 7")
Check '  the YouTube Music row filtered out: no selection' "[$(Get-SelectedRows $h)]" '[]'
Choose-Source $h 0
Check '  back to All: the track filtered out stays unselected' "[$(Get-SelectedRows $h)]" '[]'

# Another tab or scope starts with nothing selected
Select-TrackRow $h 1
Click-Tab $h 1
Check 'Synced tab: rows | tabs | result line' "$(Get-TrackRows $h)| $(Get-TrackSummary $h)" '| All 7 | Synced 0 | Waiting 7 | Needs review 0 | Failed 0 | Showing 0 of 7'
Click-Tab $h 2
Check 'Waiting tab: rows' (Get-TrackRows $h) "$RowC / $RowSenorita / $RowB / $RowSharedAlone / $RowA / $RowLate / $RowSlow"
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

"--- Run 4: Spotify pages and errors"
Write-SyncPlaylists "spotify:$BigSpotifyId", "spotify:$MissingSpotifyId", "spotify:$ErrorSpotifyId"
Remove-Item "$StateFile.bak" -ErrorAction SilentlyContinue
$app = Start-App
$h = $app.Hwnd

# The first page shows while the second is on its way; Cancel keeps it
[void]$Web.SpotifyPageGate.Reset()
$queries = Get-SpotifyQueries
Send-Command $h 'Sync &all'
Wait-SpotifyQueries ($queries + 2)
Check 'Second page asked for: the first one saved, not synced yet' (Get-BigPlaylist) "Big One 100 spBigTrack000000000001 - synced False error []"
Check '  syncing | tabs | result line' "$(Test-Syncing $h) | $(Get-TrackSummary $h)" 'True | All 100 | Synced 0 | Waiting 100 | Needs review 0 | Failed 0 | Showing 100 of 100'
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-page.png") }
Send-Command $h '&Cancel'
for ($i = 0; $i -lt 50 -and (Test-Syncing $h); $i++) { Start-Sleep -Milliseconds 100 }
Check '  Cancel: the page read stays, no error' (Get-BigPlaylist) "Big One 100 spBigTrack000000000001 - synced False error []"
Check '  syncing | tabs | result line' "$(Test-Syncing $h) | $(Get-TrackSummary $h)" 'False | All 100 | Synced 0 | Waiting 100 | Needs review 0 | Failed 0 | Showing 100 of 100'

[void]$Web.SpotifyPageGate.Set()
# The second page fails once, as Spotify's servers do now and then
$Web.SpotifyFailures["$BigSpotifyId 100"] = 1
$queries = Get-SpotifyQueries
Send-Command $h 'Sync &all'
Wait-Fetches $h $queries 4 -Spotify
Check 'Sync all: two pages for 150 tracks, the second asked for again after HTTP 500, one refused query, none without a page' (
	@($Web.SpotifyQueries() | Select-Object -Skip $queries) -join ' / ') (
	"$BigSpotifyId 0 100 Bearer fake-token-$BigSpotifyId WebPlayer / $BigSpotifyId 100 100 Bearer fake-token-$BigSpotifyId WebPlayer / " +
	"$BigSpotifyId 100 100 Bearer fake-token-$BigSpotifyId WebPlayer / $ErrorSpotifyId 0 100 Bearer fake-token-$ErrorSpotifyId WebPlayer")
$big = (Get-SavedState).playlists."spotify:$BigSpotifyId"
Check '  the big playlist: name, sources, first and last' "$($big.name) $(@($big.sources).Count) $($big.sources[0]) $($big.sources[149])" (
	'Big One 150 spotify:spBigTrack000000000001 spotify:spBigTrack000000000150')
Check '  synced, no error' (Get-BigPlaylist) "Big One 150 spBigTrack000000000001 spBigTrack000000000101 synced True error []"
Check '  the failures' (@((Get-SavedPlaylistSync) -split ' / ' | Where-Object { $_ -notlike "spotify:$BigSpotifyId*" }) -join ' / ') (
	"spotify:$MissingSpotifyId [Named $MissingSpotifyId]  synced False error: The playlist doesn't exist or is private. at True / " +
	"spotify:$ErrorSpotifyId [Named $ErrorSpotifyId]  synced False error: Spotify said: PersistedQueryNotFound at True")
Check '  tabs | result line' (Get-TrackSummary $h) 'All 150 | Synced 0 | Waiting 150 | Needs review 0 | Failed 0 | Showing 150 of 150'
Check '  infobar: the first failure' (Get-InfobarText $h) (
	"True Couldn$([char]0x2019)t fetch $([char]0x201C)Named $MissingSpotifyId$([char]0x201D). It may be private or deleted.")

# The playlist changes: 110 new tracks, then the first 40 it had. Between the pages, its earlier tracks stay after the
# new ones, so none leaves; at the end, the ones it dropped go.
$Web.SpotifyItems[$BigSpotifyId] = [Collections.Generic.List[string]]@(@(151..260) + @(1..40) | ForEach-Object { New-BigTrack $_ })
[void]$Web.SpotifyPageGate.Reset()
$queries = Get-SpotifyQueries
Send-Command $h 'Sync &all'
Wait-SpotifyQueries ($queries + 2)
Check 'A changed list, first page: new tracks, then the earlier ones' (Get-BigPlaylist) "Big One 250 spBigTrack000000000151 spBigTrack000000000001 synced True error []"
Check '  tabs | result line' (Get-TrackSummary $h) 'All 250 | Synced 0 | Waiting 250 | Needs review 0 | Failed 0 | Showing 250 of 250'
[void]$Web.SpotifyPageGate.Set()
Wait-Fetches $h $queries 3 -Spotify
Check '  all pages: the dropped tracks gone' (Get-BigPlaylist) "Big One 150 spBigTrack000000000151 spBigTrack000000000251 synced True error []"
Check '  tabs | result line' (Get-TrackSummary $h) 'All 150 | Synced 0 | Waiting 150 | Needs review 0 | Failed 0 | Showing 150 of 150'

# A page that fails again when asked for once more fails the fetch; the page before it stays
$Web.SpotifyFailures["$BigSpotifyId 100"] = 2
$queries = Get-SpotifyQueries
Send-Command $h 'Sync &all'
Wait-Fetches $h $queries 4 -Spotify
Check 'HTTP 500 twice: the second page asked for twice' (@($Web.SpotifyQueries() | Select-Object -Skip $queries | ForEach-Object { ($_ -split ' ')[0..1] -join ' ' }) -join ' / ') (
	"$BigSpotifyId 0 / $BigSpotifyId 100 / $BigSpotifyId 100 / $ErrorSpotifyId 0")
Check '  the fetch failed, its tracks kept' (Get-BigPlaylist) "Big One 150 spBigTrack000000000151 spBigTrack000000000251 synced True error [HTTP 500 from 127.0.0.1]"
Stop-App $app
