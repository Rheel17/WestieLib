# Syncing playlists: Sync all, Sync selected and Sync now fetch YouTube Music (from the fake yt-dlp) and Spotify (from
# FakeWeb) playlists into state.json and the track table, a failed fetch keeps its error, Cancel stops a fetch, removing
# a playlist takes its tracks along, a fetch waits for yt-dlp to be installed, and a Spotify fetch shows each page as it
# comes and fails as Spotify does. The track card shows the YouTube Music id of each track a state.json holds, and why a match
# failed. Dot-sourced by ui-checks.ps1.

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

# Each Spotify source with a match as "id ytmId confidence"
function Get-SavedMatches {
	@((Get-SavedState).sources.PSObject.Properties | Where-Object { $_.Value.match } | ForEach-Object {
		"$($_.Name -replace $KeyPrefix, '') $($_.Value.match.ytmId) $($_.Value.match.confidence)"
	}) -join ' / '
}

# The same with the match's doubts, as "id ytmId confidence [doubts]", for the sources whose id matches $pattern
function Get-SavedJudgements([string]$pattern) {
	@((Get-SavedState).sources.PSObject.Properties | Where-Object { $_.Value.match -and ($_.Name -replace $KeyPrefix, '') -match $pattern } | ForEach-Object {
		"$($_.Name -replace $KeyPrefix, '') $($_.Value.match.ytmId) $($_.Value.match.confidence) [$(@($_.Value.match.doubts) -join ',')]"
	}) -join ' / '
}

# Each failed track as "id failure [error, its lines separated by |]"
function Get-SavedFailures {
	@((Get-SavedState).tracks.PSObject.Properties | Where-Object { $_.Value.failure } | ForEach-Object {
		"$($_.Name -replace $KeyPrefix, '') $($_.Value.failure) [$($_.Value.error -replace "`n", ' | ')]"
	}) -join ' / '
}

function Get-YtmQueriesSince([int]$count) { @($Web.YtmQueries() | Select-Object -Skip $count) -join ' / ' }

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

# The tooltip of row $index's Status cell, the column at display position $position, once the mouse moves onto it
# (HDM_ORDERTOINDEX, HDM_GETITEMRECT, LVM_GETITEMRECT)
function Get-StatusTip($hwnd, [int]$index, [int]$position = 0) {
	$list = Get-TrackTable $hwnd
	$header = [W]::SendMessageW($list, 0x101F, [IntPtr]::Zero, [IntPtr]::Zero)
	$column = [W]::SendForRect($header, 0x1207, [W]::SendMessageW($header, 0x120F, [IntPtr]$position, [IntPtr]::Zero))
	$row = [W]::SendForRect($list, 0x100E, [IntPtr]$index)
	[void][W]::SendMessageW($list, 0x200, [IntPtr]::Zero, (LParam ([int](($column.Left + $column.Right) / 2)) ([int](($row.Top + $row.Bottom) / 2))))
	[W]::TooltipText($list, 1)
}

# The first row in view (LVM_GETTOPINDEX)
function Get-TopRow($hwnd) { [int][W]::SendMessageW((Get-TrackTable $hwnd), 0x1027, [IntPtr]::Zero, [IntPtr]::Zero) }

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

function Get-LogFiles { @(Get-ChildItem (Join-Path $DataDir 'logs') -Filter '*.log' | Sort-Object Name) }

# The newest log file's lines, with T for the time, whose format the unit checks cover
function Get-LogLines { @(Get-Content (Get-LogFiles)[-1].FullName -Encoding UTF8 | ForEach-Object { $_ -replace '^\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d{3} ', 'T ' }) }

# Its lines of the levels $levels names (INFO|ERROR, say)
function Get-LogLevelLines([string]$levels) { @(Get-LogLines | Where-Object { $_ -match "^T ($levels) " }) -join ' / ' }

# A tool run in it, from the line naming $pattern to the end of its indented lines
function Get-LogToolRun([string]$pattern) {
	$lines = Get-LogLines
	$start = [array]::FindIndex([string[]]$lines, [Predicate[string]]{ param($line) $line -match "^T TOOL +.*$pattern" })
	if ($start -lt 0) { return '' }
	$end = $start + 1
	while ($end -lt $lines.Count -and $lines[$end].StartsWith(' ')) { $end++ }
	$lines[$start..($end - 1)] -join ' / '
}

function Get-TrackSummary($hwnd) { "$([W]::TabTexts((Get-StatusTabs $hwnd))) | $([W]::TextOf((Get-ResultLine $hwnd)))" }

$Cafe = "Caf$([char]0xE9) Lights"
$TracksName = 'Checked PLtracks1'
$SharedName = 'Checked PLshared1'
$SlowName = 'Checked PLslow1'
$BrokenError = 'Unable to download API page: HTTP Error 503: Service Unavailable'
$SyncSpotifyKey = "spotify:$SpotifyId"
$FetchMessage = "Couldn't fetch `"Named PLbroken1`". It may be private or deleted."
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
# A match from before the Matcher stored its doubts
$LowConfidence = "$Bullet Low confidence"

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
Check '  sources: Topic dropped, a private entry, an episode and a local file left out, albums from Spotify only' (Get-SavedSources) (
	'spTrackLateNight000001 Late Night Swing [Ana, Ben] (Nights) 241000 > spTrackLateNight000001 / ' +
	"spTrackSenorita0000002 $Senorita [Earth, Wind & Fire] (Summer) 185000 > spTrackSenorita0000002 / " +
	"AAAAA1 Way Down We Go [KALEO] 214000 > AAAAA1 / BBBBB2 $Cafe [Nova] 191000 > BBBBB2 / " +
	'CCCCC4 No Length [Somebody] 0 > CCCCC4 / DDDDD6 Only Here [Solo] 65000 > DDDDD6 / SHARE5 Shared Song [Both] 200000 > SHARE5')
Check '  tracks' (Get-SavedTracks) ('spTrackLateNight000001 pending [spTrackLateNight000001] True / spTrackSenorita0000002 pending [spTrackSenorita0000002] True / ' +
	'AAAAA1 pending [AAAAA1] True / BBBBB2 pending [BBBBB2] True / CCCCC4 pending [CCCCC4] True / DDDDD6 pending [DDDDD6] True / SHARE5 pending [SHARE5] True')
Check '  the Spotify tracks searched on YouTube Music, once each' ($Web.YtmQueries() -join ' / ') "Ana Ben Late Night Swing / Earth, Wind & Fire $Senorita"
Check '  All tracks: rows, by title' (Get-TrackRows $h) "$RowB / $RowLate / $RowC / $RowD / $RowSenorita / $RowShared / $RowA"
Check '  tabs | result line' (Get-TrackSummary $h) 'All 7 | Synced 0 | Waiting 7 | Needs review 0 | Failed 0 | Showing 7 of 7'
Check '  Library card' (Get-CardTexts $h) "Library card: All tracks / Library / Folder / $DefaultLibrary / Tracks / 7 tracks, 0 synced / Open library folder"
Check '  syncing after it' (Test-Syncing $h) 'False'
# The rows with tracks waiting have a line for them
$script:TallRows = 1, 2, 3
Check-ListButton '  rows 1 to 3 a line taller: Add playlist under them' $h 4
Check '  infobar: the failed fetch' (Get-InfobarText $h) "True $FetchMessage"
Check '  the rows sync controls tooltips' "$([W]::TooltipText((Get-PlaylistList $h), 0)) / $([W]::TooltipText((Get-PlaylistList $h), 3))" "Sync $TracksName / Sync Named PLbroken1"

# The log file has what the sync did, and each yt-dlp run
Check 'Log: one file, named by the time' "$((Get-LogFiles).Count) $((Get-LogFiles)[0].Name -match '^\d{4}-\d\d-\d\d_\d\d-\d\d-\d\d\.log$')" '1 True'
Check '  what the sync did' (Get-LogLevelLines 'INFO|WARNING|ERROR') (
	'T INFO    Sync started: 4 playlists / ' +
	"T INFO    Fetched 4 tracks [$TracksName; playlist ytm:PLtracks1] / " +
	"T INFO    Fetched 2 tracks [$SpotifyName; playlist $SyncSpotifyKey] / " +
	"T INFO    Fetched 2 tracks [$SharedName; playlist ytm:PLshared1] / " +
	"T ERROR   Couldn't fetch the playlist: $BrokenError [Named PLbroken1; playlist ytm:PLbroken1] / " +
	'T INFO    Matched "Late Night Swing" by Ana, Ben (100%) [Late Night Swing; track spotify:spTrackLateNight000001] / ' +
	"T INFO    Matched `"$Senorita`" by Earth, Wind & Fire (100%) [$Senorita; track spotify:spTrackSenorita0000002] / " +
	'T INFO    Sync finished')
Check '  a failed yt-dlp run: its command, exit code and stderr' (Get-LogToolRun 'PLbroken1$') (
	'T TOOL    yt-dlp --ignore-config --flat-playlist --dump-single-json https://music.youtube.com/playlist?list=PLbroken1 / ' +
	'    exit code 1 / ' +
	'    stderr: / ' +
	'        WARNING: [youtube:tab] YouTube Music is not directly supported / ' +
	"        ERROR: [youtube:tab] PLbroken1: $BrokenError")
Check '  a successful one: only the size of its output' ((Get-LogToolRun 'PLtracks1$') -replace '\d+ bytes', 'N bytes') (
	'T TOOL    yt-dlp --ignore-config --flat-playlist --dump-single-json https://music.youtube.com/playlist?list=PLtracks1 / ' +
	'    exit code 0 / ' +
	'    stdout: N bytes')
Check '  the three runs at Debug too' (@(Get-LogLines | Where-Object { $_ -like 'T DEBUG   yt-dlp *: exit code *' }).Count) 3
Check '  each search at Debug, with its best result' (@(Get-LogLines | Where-Object { $_ -like 'T DEBUG   Searched *' }) -join ' / ') (
	'T DEBUG   Searched "Ana Ben Late Night Swing": 1 result, best "Late Night Swing" by Ana, Ben (100%) [Late Night Swing; track spotify:spTrackLateNight000001] / ' +
	"T DEBUG   Searched `"Earth, Wind & Fire $Senorita`": 1 result, best `"$Senorita`" by Earth, Wind & Fire (100%) [$Senorita; track spotify:spTrackSenorita0000002]")

# The log pane has the same lines, from Info up; the error in red
$FetchedRows = "T | Info | $TracksName | Fetched 4 tracks / T | Info | $SpotifyName | Fetched 2 tracks / T | Info | $SharedName | Fetched 2 tracks"
$ErrorRow = "T | Error | Named PLbroken1 | Couldn't fetch the playlist: $BrokenError"
$LateRow = 'T | Info | Late Night Swing | Matched "Late Night Swing" by Ana, Ben (100%)'
$SenoritaRow = "T | Info | $Senorita | Matched `"$Senorita`" by Earth, Wind & Fire (100%)"
Send-Command $h '&Log'
Start-Sleep -Milliseconds 200
$logList = Get-LogList $h
Check 'Log pane: the lines from Info up' (Get-LogRows $h) (
	"T | Info | $([char]0x2014) | Sync started: 4 playlists / $FetchedRows / $ErrorRow / $LateRow / $SenoritaRow / T | Info | $([char]0x2014) | Sync finished")
Check '  the times, to the millisecond' (@(0..7 | Where-Object { ([W]::ListViewRow($logList, $_) -split ' \| ')[0] -match '^\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d{3}$' }).Count) 8
Check '  the error in red, no warning' "$([W]::RedInk($h, $logList)) $([W]::AmberInk($h, $logList))" 'True False'
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-log.png") }
Choose-LogLevel $h 3
Check 'Errors only' (Get-LogRows $h) $ErrorRow
Choose-LogLevel $h 0
Check 'Debug and up: the yt-dlp runs too' (@((Get-LogRows $h) -split ' / ' | Where-Object { $_ -like 'T | Debug | *yt-dlp --ignore-config*' }).Count) 3
Choose-LogLevel $h 1
Click-LogScope $h 1
Check 'Selected playlist, with All tracks: every line' (@((Get-LogRows $h) -split ' / ').Count) 8
Click-PlaylistRow $h 1
Check '  the first playlist: its line' (Get-LogRows $h) "T | Info | $TracksName | Fetched 4 tracks"
Click-PlaylistRow $h 2
Check '  the Spotify playlist: its line and its tracks''' (Get-LogRows $h) "T | Info | $SpotifyName | Fetched 2 tracks / $LateRow / $SenoritaRow"
Click-LogScope $h 2
Check 'Selected track, none selected' "[$(Get-LogRows $h)] $([W]::TextOf($logList))" '[] Select a track to see its lines.'
Select-TrackRow $h 0
# The selection reaches the log on a posted message
Start-Sleep -Milliseconds 200
Check '  Late Night Swing: its line' "$(Get-LogRows $h) [$([W]::TextOf($logList))]" "$LateRow []"
Click-LogScope $h 0
Click-AllTracks $h
Send-Command $h '&Log'
Start-Sleep -Milliseconds 200

# One selected track shows its card; a link to one of its playlists selects that playlist
Select-TrackRow $h 5
Check 'Shared Song selected: its card' (Wait-CardTexts $h 'Track card') ("Track card:  / Shared Song / Both / No album yet / " +
	"Status / Waiting / Downloads on the next sync. / Source / Source / YouTube Music / Playlists / $TracksName, $SharedName / " +
	"Key / ytm:trackSHARE5 / Audio / ID / trackSHARE5 / Chosen by / Playlist / File / Not downloaded yet.")
Select-TrackRow $h 1
Check 'Late Night Swing selected: its card' (Wait-TrackCard $h 'spotify:spTrackLateNight000001') ("Track card:  / Late Night Swing / Ana, Ben / Nights / " +
	"Status / Waiting / Downloads on the next sync. / Source / Source / Spotify / Playlists / $SpotifyName / " +
	"Key / spotify:spTrackLateNight000001 / Audio / Matched / Late Night Swing / Artists / Ana, Ben / Length / 4:01 (Spotify 4:01) / " +
	"ID / ytspTrackLateNight000001 / Chosen by / Matcher / Confidence / 100% / File / Not downloaded yet.")
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

# Several selected tracks show the selection card
Select-TrackRow $h 0
[W]::PostShiftKey((Get-TrackTable $h), 0x28)
Check 'Shift+Down: two rows selected' (Get-SelectedRows $h) "0 $Cafe / 1 Late Night Swing"
Check '  the selection card' (Wait-CardTexts $h 'Selection card') 'Selection card: 2 tracks selected / Status / Waiting / 2'
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-selection.png") }
$table = Get-TrackTable $h
[W]::PostCtrlKey($table, 0x41)
$count = [int][W]::SendMessageW($table, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero)
Check 'Ctrl+A: every row selected' "$(@((Get-SelectedRows $h) -split ' / ').Count) of $count | $(Wait-CardTexts $h 'Selection card')" (
	"$count of $count | Selection card: $count tracks selected / Status / Waiting / $count")

Click-PlaylistRow $h 1
Check 'Playlist 1: rows by position, a repeat once' (Get-TrackRows $h) "1 | $RowA / 2 | $RowB / 3 | $RowC / 4 | $RowShared"
Check '  tabs | result line' (Get-TrackSummary $h) "All 4 | Synced 0 | Waiting 4 | Needs review 0 | Failed 0 | Duplicates 0 | Showing 4 of 7 in $TracksName"
Check '  card' ((Get-CardTexts $h) -replace 'today \d\d:\d\d', 'today HH:MM') ("Playlist card: $TracksName / YouTube Music playlist / Playlist / Link / PLtracks1 / " +
	"Tracks / 4 / Synced / 0/4 / Waiting / 4 / Shared / 1 / Last sync / today HH:MM / Sync playlist / Remove playlist$Ellipsis")
Click-PlaylistRow $h 2
Check 'The Spotify playlist: rows by position, a repeat once' (Get-TrackRows $h) "1 | $RowLate / 2 | $RowSenorita"
Check '  tabs | result line' (Get-TrackSummary $h) "All 2 | Synced 0 | Waiting 2 | Needs review 0 | Failed 0 | Duplicates 0 | Showing 2 of 7 in $SpotifyName"
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
Send-Click ([W]::Children($infobar, 'WestieLib.CloseButton'))[0] (S 12) (S 12)
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
Check '  tabs | result line' (Get-TrackSummary $h) "All 1 | Synced 0 | Waiting 1 | Needs review 0 | Failed 0 | Duplicates 0 | Showing 1 of 8 in $SlowName"
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

# A header click sorts the rows by its column, a second one turns them round; rows that tie keep the order they had
Click-Header $h 5
Check 'Sorted by Time' (Get-TrackRows $h) "$RowC / $RowSenorita / $RowB / $RowSharedAlone / $RowA / $RowLate / $RowSlow"
Click-Header $h 5
Check '  again: longest first' (Get-TrackRows $h) "$RowSlow / $RowLate / $RowA / $RowSharedAlone / $RowB / $RowSenorita / $RowC"
Click-Header $h 2
Check 'Sorted by Artists' (Get-TrackRows $h) "$RowLate / $RowSharedAlone / $RowSenorita / $RowA / $RowB / $RowC / $RowSlow"
Click-Header $h 3
Check 'Sorted by Album: the YouTube Music rows have none, so stay by artists' (Get-TrackRows $h) (
	"$RowSharedAlone / $RowA / $RowB / $RowC / $RowSlow / $RowLate / $RowSenorita")
Click-Header $h 0
Check 'Sorted by Status: all wait, so stay by album' (Get-TrackRows $h) "$RowSharedAlone / $RowA / $RowB / $RowC / $RowSlow / $RowLate / $RowSenorita"

# The selection stays with its track when the rows move or narrow
Select-TrackRow $h 4
Check 'Select row 5' (Get-SelectedRows $h) '4 Slow One'
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

# The search: each word somewhere in the title, artists or album, in any order, case and accents aside. The playlists
# aren't searched, so "late" leaves out the rows of Late Night Cafe.
$search = Get-SearchField $h
foreach ($case in @(
	@{Text = 'CAFE'; Rows = $RowB}, @{Text = 'senorita'; Rows = $RowSenorita}, @{Text = "Se$([char]0xD1)ORITA"; Rows = $RowSenorita},
	@{Text = ' swing  ana '; Rows = $RowLate}, @{Text = 'summer'; Rows = $RowSenorita}, @{Text = 'late'; Rows = $RowLate}
)) {
	[W]::SetEditText($search, $case.Text)
	Check "Search '$($case.Text)': rows" (Get-TrackRows $h) $case.Rows
}
Check '  tabs | result line' (Get-TrackSummary $h) 'All 1 | Synced 0 | Waiting 1 | Needs review 0 | Failed 0 | Showing 1 of 7'
[W]::SetEditText($search, 'zzz')
Check "Search 'zzz': rows | tabs | result line" "[$(Get-TrackRows $h)] | $(Get-TrackSummary $h)" '[] | All 0 | Synced 0 | Waiting 0 | Needs review 0 | Failed 0 | Showing 0 of 7'
[W]::SetEditText($search, 'night')
Choose-Source $h 2
Check "Search 'night' and Source Spotify" (Get-TrackRows $h) $RowLate
Choose-Source $h 1
Check '  and Source YouTube Music' "[$(Get-TrackRows $h)]" '[]'
Choose-Source $h 0

# The selection stays with its track while the search keeps it
[void][W]::SendMessageW($search, 0x102, [IntPtr]0x1B, [IntPtr]1)
Check 'Esc: the search empties, every row back' "'$([W]::EditText($search))' $(Get-TrackRows $h)" (
	"'' $RowC / $RowSenorita / $RowB / $RowSharedAlone / $RowA / $RowLate / $RowSlow")
Select-TrackRow $h 6
[W]::SetEditText($search, 'o')
Check "Search 'o': the selected row kept" (Get-SelectedRows $h) '5 Slow One'
[W]::SetEditText($search, 'turtle')
Check "  'turtle', its artist: kept" (Get-SelectedRows $h) '0 Slow One'
[W]::SetEditText($search, 'cafe')
Check "  'cafe': filtered out, so unselected" "[$(Get-SelectedRows $h)]" '[]'
[void][W]::SendMessageW($search, 0x102, [IntPtr]0x1B, [IntPtr]1)

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
$logged = Get-LogLevelLines 'INFO|WARNING'
Check '  log: the wait, the install, the fetch' (
	"$($logged -match 'T WARNING Waiting for yt-dlp to install \[Named PLtracks1; playlist ytm:PLtracks1\]') " +
	"$($logged -match 'T INFO    Installed yt-dlp \S+') $($logged -match "T INFO    Fetched 4 tracks \[$TracksName; playlist ytm:PLtracks1\]")") 'True True True'
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
	"True Couldn't fetch `"Named $MissingSpotifyId`". It may be private or deleted.")

# A sync refreshes the rows without scrolling the table; another tab starts it at the top
$list = Get-TrackTable $h
[void][W]::SendMessageW($list, 0x1013, [IntPtr]120, [IntPtr]0)
$top = Get-TopRow $h
$queries = Get-SpotifyQueries
Send-Command $h 'Sync &all'
Wait-Fetches $h $queries 3 -Spotify
Check 'Scrolled down, then a sync: the table stays there' "$(Get-TopRow $h) $($top -gt 0)" "$top True"
Click-Tab $h 2
Check '  the Waiting tab: at the top' (Get-TopRow $h) 0
Click-Tab $h 0

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

"--- Run 5: matches and failures in state.json"
# A YouTube Music playlist and a Spotify one: a track only on YouTube Music, two the Matcher matched (one a live version
# 33 s longer), one on both whose match is the YouTube Music source, one without a match, one whose search failed
$MatchSpotifyKey = 'spotify:4matchmatchmatchmatchm'
$NoMatchError = 'Searched \"The Late Hours No Results (WCS Edit)\": no results\nSearched \"The Late Hours No Results\": no usable results'
$SearchError = 'Couldn''t reach music.youtube.com: The server name or address could not be resolved'
# Each element in parentheses: a comma binds tighter than +
$sources = @(
	('"spotify:spBestPart000000000001": {"title": "Best Part", "artists": ["Daniel Caesar", "H.E.R."], "album": "Freudian", "durationMs": 209433, ' +
		'"match": {"ytmId": "zNhtz0Lygik", "confidence": 0.94, "title": "Best Part (feat. H.E.R.)", "artists": ["Daniel Caesar"], "durationMs": 210000}, ' +
		'"track": "spotify:spBestPart000000000001"}'),
	('"spotify:spCrazy0000000000000002": {"title": "Crazy", "artists": ["Gnarls Barkley"], "album": "St. Elsewhere", "durationMs": 178000, ' +
		'"match": {"ytmId": "liveCrazy01", "confidence": 0.61, "title": "Crazy (Live at Coachella 2006)", "artists": ["Gnarls Barkley"], "durationMs": 211000}, ' +
		'"track": "spotify:spCrazy0000000000000002"}'),
	('"spotify:spMerged000000000000003": {"title": "Merged Song", "artists": ["Both"], "album": "Shared", "durationMs": 200000, ' +
		'"match": {"ytmId": "vidMerged01", "confidence": 0.96, "title": "Merged Song", "artists": ["Both"], "durationMs": 201000}, "track": "ytm:vidMerged01"}'),
	('"spotify:spMergedAgain000000006": {"title": "Merged Song", "artists": ["Both"], "album": "Shared (Deluxe)", "durationMs": 200000, ' +
		'"match": {"ytmId": "vidMerged01", "confidence": 0.9, "title": "Merged Song", "artists": ["Both"], "durationMs": 201000}, "track": "ytm:vidMerged01"}'),
	'"spotify:spNoResults00000000004": {"title": "No Results", "artists": ["The Late Hours"], "album": "Sway", "durationMs": 228000, "track": "spotify:spNoResults00000000004"}',
	'"spotify:spOffline0000000000005": {"title": "Offline", "artists": ["Nobody"], "album": "Unplugged", "durationMs": 150000, "track": "spotify:spOffline0000000000005"}',
	'"ytm:vidAlone001": {"title": "Alone Here", "artists": ["Lone"], "durationMs": 180000, "track": "ytm:vidAlone001"}',
	'"ytm:vidMerged01": {"title": "Merged Song", "artists": ["Both"], "durationMs": 201000, "track": "ytm:vidMerged01"}'
)
$tracks = @(
	'"spotify:spBestPart000000000001": {"sources": ["spotify:spBestPart000000000001"], "status": "pending", "addedAt": "2026-01-02T12:00:00Z"}',
	'"spotify:spCrazy0000000000000002": {"sources": ["spotify:spCrazy0000000000000002"], "status": "pending", "addedAt": "2026-01-02T12:00:00Z"}',
	('"spotify:spNoResults00000000004": {"sources": ["spotify:spNoResults00000000004"], "status": "failed", "failure": "noMatch", ' +
		"`"error`": `"First line, left out\n$NoMatchError`", `"addedAt`": `"2026-01-02T12:00:00Z`"}"),
	("`"spotify:spOffline0000000000005`": {`"sources`": [`"spotify:spOffline0000000000005`"], `"status`": `"failed`", `"failure`": `"search`", " +
		"`"error`": `"$SearchError`", `"addedAt`": `"2026-01-02T12:00:00Z`"}"),
	'"ytm:vidAlone001": {"sources": ["ytm:vidAlone001"], "status": "pending", "addedAt": "2026-01-01T12:00:00Z"}',
	('"ytm:vidMerged01": {"sources": ["ytm:vidMerged01", "spotify:spMerged000000000000003", "spotify:spMergedAgain000000006"], "status": "pending", ' +
		'"addedAt": "2026-01-01T12:00:00Z"}')
)
$playlists = '"ytm:PLmatch1": {"url": "https://music.youtube.com/playlist?list=PLmatch1", "name": "WCS", "addedAt": "2026-01-01T12:00:00Z", ' +
	'"sources": ["ytm:vidAlone001", "ytm:vidMerged01"]}, ' +
	"`"$MatchSpotifyKey`": {`"url`": `"https://open.spotify.com/playlist/4matchmatchmatchmatchm`", `"name`": `"Her playlist`", " +
	'"addedAt": "2026-01-02T12:00:00Z", "sources": ["spotify:spBestPart000000000001", "spotify:spCrazy0000000000000002", ' +
	'"spotify:spMerged000000000000003", "spotify:spNoResults00000000004", "spotify:spOffline0000000000005", "spotify:spMergedAgain000000006"]}'
[IO.File]::WriteAllText($StateFile, "{`"version`": 1, `"playlists`": {$playlists}, `"sources`": {$($sources -join ', ')}, `"tracks`": {$($tracks -join ', ')}}")
Remove-Item "$StateFile.bak" -ErrorAction SilentlyContinue
$seeded = Get-SavedState
$app = Start-App
$h = $app.Hwnd
Check 'All tracks: a row per track, two failed' (Get-TrackSummary $h) 'All 6 | Synced 0 | Waiting 4 | Needs review 1 | Failed 2 | Showing 6 of 6'

Select-TrackRow $h 0
Check 'Only on YouTube Music: the playlist chose its id' (Wait-TrackCard $h 'ytm:vidAlone001') ("Track card:  / Alone Here / Lone / No album yet / " +
	"Status / Waiting / Downloads on the next sync. / Source / Source / YouTube Music / Playlists / WCS / Key / ytm:vidAlone001 / " +
	"Audio / ID / vidAlone001 / Chosen by / Playlist / File / Not downloaded yet.")
Select-TrackRow $h 1
Check 'Matched: what the Matcher chose' (Wait-TrackCard $h 'spotify:spBestPart000000000001') ("Track card:  / Best Part / Daniel Caesar, H.E.R. / Freudian / " +
	"Status / Waiting / Downloads on the next sync. / Source / Source / Spotify / Playlists / Her playlist / " +
	"Key / spotify:spBestPart000000000001 / Audio / Matched / Best Part (feat. H.E.R.) / Artists / Daniel Caesar / Length / 3:30 (Spotify 3:29) / " +
	"ID / zNhtz0Lygik / Chosen by / Matcher / Confidence / 94% / File / Not downloaded yet.")
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-matched.png") }
# The bar is 110 long, 94% of it green, then 6 to the percentage
$confidence = ([W]::Children((Get-InspectorCard $h), 'WestieLib.ConfidenceValue'))[0]
$y = [int]((Get-ClientHeight $confidence) / 2)
Check '  confidence bar: filled, empty end, gap' ([W]::PixelsIn($h, $confidence, @(0, $y, ((S 110) - 1), $y, ((S 110) + 1), $y)) -join ' ') '06b025 e6e6e6 ffffff'
Select-TrackRow $h 2
Check 'A live version: lengths 33 s apart, 61% needs review' (Wait-TrackCard $h 'spotify:spCrazy0000000000000002') ("Track card:  / Crazy / Gnarls Barkley / St. Elsewhere / " +
	"Status / Needs review / $LowConfidence / $ChooseMatch / Source / Source / Spotify / Playlists / Her playlist / " +
	"Key / spotify:spCrazy0000000000000002 / Audio / Matched / Crazy (Live at Coachella 2006) / Artists / Gnarls Barkley / Length / 3:31 (Spotify 2:58) / " +
	"ID / liveCrazy01 / Chosen by / Matcher / Confidence / 61% / File / Not downloaded yet.")
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-live.png") }
$confidence = ([W]::Children((Get-InspectorCard $h), 'WestieLib.ConfidenceValue'))[0]
Check '  confidence bar: yellow' ([W]::PixelsIn($h, $confidence, @(0, $y)) -join ' ') 'd9a400'
# The merged track has one row, for its first entry: WCS's, the first playlist
Select-TrackRow $h 3
Check 'On both: one row, the playlist chose, the match agrees' (Wait-TrackCard $h 'ytm:vidMerged01') ("Track card:  / Merged Song / Both / Shared / " +
	"Status / Waiting / Downloads on the next sync. / Sources / Listed on / YouTube Music /  / Spotify / " +
	"Playlists / WCS, Her playlist / Key / ytm:vidMerged01 / Audio / ID / vidMerged01 / Chosen by / Playlist / " +
	"Spotify match / Same ID (96%) / File / Not downloaded yet.")
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-merged.png") }
Check '  no duplicate in All tracks: its Status tooltip' (Get-StatusTip $h 3) 'Waiting'
Select-TrackRow $h 4
Check 'No match: the reason, no output' (Wait-TrackCard $h 'spotify:spNoResults00000000004') ("Track card:  / No Results / The Late Hours / Sway / " +
	"Status / Failed / No match found. / No match on YouTube Music / $ChooseMatch / Source / Source / Spotify / Playlists / Her playlist / " +
	"Key / spotify:spNoResults00000000004 / Audio / ID / No match found / File / Not downloaded yet.")
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-nomatch.png") }
$card = Get-InspectorCard $h
$labels = @(Get-DirectChildren $card 'WestieLib.Label' | Where-Object { [W]::IsWindowVisible($_) })
$explanation = [W]::RectIn(($labels | Where-Object { [W]::TextOf($_) -eq 'No match found.' })[0], $card)
$reasonLabel = ($labels | Where-Object { [W]::TextOf($_) -eq 'No match on YouTube Music' })[0]
$reason = [W]::RectIn($reasonLabel, $card)
Check '  reason 8 under the explanation, no box' "$($reason.Top - $explanation.Bottom) $([W]::IsWindowVisible(([W]::Children($card, 'WestieLib.OutputBox'))[0]))" (
	"$(S 8) False")
$noId = ($labels | Where-Object { [W]::TextOf($_) -eq 'No match found' })[0]
Check '  reason and ID in red' "$([W]::RedInk($h, $reasonLabel)) $([W]::RedInk($h, $noId))" 'True True'
Select-TrackRow $h 5
Check 'The search failed: its error' (Wait-TrackCard $h 'spotify:spOffline0000000000005') ("Track card:  / Offline / Nobody / Unplugged / " +
	"Status / Failed / Retried on the next sync. / Couldn't search YouTube Music / $SearchError / " +
	"Source / Source / Spotify / Playlists / Her playlist / Key / spotify:spOffline0000000000005 / " +
	"Audio / ID / Not chosen yet / File / Not downloaded yet.")

# Her playlist lists the merged track twice, through two Spotify releases: the second is a duplicate of the first. The
# first isn't one, although WCS lists the track before it.
$script:TallRows = 1, 2
Click-PlaylistRow $h 2
$merged = "Merged Song | Both | Shared | WCS, Her playlist | 3:20"
Check 'Her playlist: a row per entry, Playlists naming WCS too' (Get-TrackRows $h) ("1 |  | Best Part | Daniel Caesar, H.E.R. | Freudian | Her playlist | 3:29 / " +
	"2 |  | Crazy | Gnarls Barkley | St. Elsewhere | Her playlist | 2:58 / 3 |  | $merged / 4 |  | No Results | The Late Hours | Sway | Her playlist | 3:48 / " +
	"5 |  | Offline | Nobody | Unplugged | Her playlist | 2:30 / 6 |  | $merged")
Check '  tabs | result line' (Get-TrackSummary $h) 'All 6 | Synced 0 | Waiting 4 | Needs review 1 | Failed 2 | Duplicates 1 | Showing 6 of 6 in Her playlist'
Check '  Status tooltips: #3 | #6' "$(Get-StatusTip $h 2 1) | $(Get-StatusTip $h 5 1)" "Waiting | Duplicate of track #3 (Waiting)"
Select-TrackRow $h 5
Check '  #6 a duplicate: its card' (Wait-TrackCard $h 'spotify:spMergedAgain000000006') ("Track card:  / Merged Song / Both / Shared / " +
	"Status / Duplicate / Duplicate of track #3 in this playlist. / " +
	"Sources / Listed on / YouTube Music /  / Spotify / Playlists / WCS, Her playlist / " +
	"Key / spotify:spMergedAgain000000006 / Audio / Matched / Merged Song / Artists / Both / Length / 3:21 (Spotify 3:20) / " +
	"ID / vidMerged01 / Chosen by / Matcher / Confidence / 90% / File / Not downloaded yet.")
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-duplicate.png") }
Click-Tab $h 5
Check '  the Duplicates tab: #6 alone' "$(Get-TrackRows $h) | $(Get-TrackSummary $h)" (
	"6 |  | $merged | All 6 | Synced 0 | Waiting 4 | Needs review 1 | Failed 2 | Duplicates 1 | Showing 1 of 6 in Her playlist")
Click-AllTracks $h
Check '  All tracks: no Duplicates tab, back to All' (Get-TrackSummary $h) 'All 6 | Synced 0 | Waiting 4 | Needs review 1 | Failed 2 | Showing 6 of 6'

# Adding a playlist saves the state, which keeps the matches and failures as they were
Click-AllTracks $h
[void](Add-Playlist $h $SpotifyLink)
for ($i = 0; $i -lt 50 -and -not (Get-SavedState).playlists.$SyncSpotifyKey; $i++) { Start-Sleep -Milliseconds 100 }
$saved = Get-SavedState
Check 'Saved again: sources and tracks unchanged' "$(($saved.sources | ConvertTo-Json -Compress -Depth 5) -eq ($seeded.sources | ConvertTo-Json -Compress -Depth 5)) $(
	($saved.tracks | ConvertTo-Json -Compress -Depth 5) -eq ($seeded.tracks | ConvertTo-Json -Compress -Depth 5))" 'True True'
Stop-App $app

"--- Run 6: matching"
# A Spotify playlist whose tracks YouTube Music's search finds: the right song second, a live version before the
# studio one, nothing at all, a search that fails, the id of a YouTube Music track, an id that a YouTube Music
# playlist added later lists, and only another song of the same artist and length. Then two the search doesn't need:
# a song another id of which a YouTube Music playlist added later lists, and one a synced playlist lists already. And
# two the search finds under other names: an artist with a word stuck to it, and a cover by the track's own artists.
# Then one each for the rules a review of real matches added: the remix by the track's remixer, a re-recording and a
# language version over the original first, an artist that shares only a word, a title written without its space, a $
# in a name, and a title that shares only half its words. Then how the Matcher decides: a guest only YouTube Music
# names (to review), the plain song after its feat. version, only a live version of another length (no match), a guest
# Spotify credits with "with" and YouTube Music with "feat.", a channel name for the artist, 7 s off on the same
# album (accepted) and on another (to review), and an explicit song after its clean version.
$MatchId = '5matchesmatchesmatches'
$Web.SpotifyNames[$MatchId] = 'Matches'
$Web.SpotifyItems[$MatchId] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Track('spClearMatch0000000001', 'Best Part', @('Daniel Caesar', 'H.E.R.'), 'Freudian', 209433),
	[FakeWeb]::Track('spLiveLoses00000000002', 'Crazy', @('Gnarls Barkley'), 'St. Elsewhere', 178000),
	[FakeWeb]::Track('spNoResults00000000003', 'Sway With Me (WCS Edit)', @('The Late Hours'), 'Sway', 228000),
	[FakeWeb]::Track('spBrokenSearch00000004', 'Offline', @('Nobody'), 'Unplugged', 150000),
	[FakeWeb]::Track('spMergesYtm00000000005', 'Way Down We Go', @('KALEO'), 'A/B', 214000),
	[FakeWeb]::Track('spJoinedLater000000006', 'Only Here', @('Solo'), 'Here', 65000),
	[FakeWeb]::Track('spOtherTitle0000000007', 'Down Home Blues', @('Kenny'), 'Blues', 180000),
	[FakeWeb]::Track('spLibraryLater00000008', 'Slow One', @('Turtle'), 'Slow', 300000),
	[FakeWeb]::Track('spLibraryFirst00000009', $Cafe, @('Nova'), 'Lights', 191000),
	[FakeWeb]::Track('spLetterArtist00000010', 'Inundated', @('Tora'), 'A Force Majeure', 182000),
	[FakeWeb]::Track('spWithTail000000000011', 'Retrograde (Feki & Glasshouse Cover)', @('Feki', 'Glass House'), 'Retrograde', 220000),
	[FakeWeb]::Track('spRuleRemixer000000012', 'Horns (StayLoose Remix)', @('Bryce Fox', 'StayLoose'), 'Horns', 204000),
	[FakeWeb]::Track('spRuleOwnVersion000013', "Everything Has Changed (Taylor's Version)", @('Taylor Swift'), 'Red', 245000),
	[FakeWeb]::Track('spRuleLanguage00000014', 'Love U Like That - Korean Version', @('Lauv'), 'Love U', 212000),
	[FakeWeb]::Track('spRuleOtherArtist00015', 'Dance Monkey', @('Tones And I'), 'Kids', 209000),
	[FakeWeb]::Track('spRuleSpacedTitle00016', 'Funkytown', @('Lipps Inc.'), 'Mouth', 240000),
	[FakeWeb]::Track('spRuleDollar0000000017', 'Tik Tok', @('Ke$ha'), 'Animal', 200000),
	[FakeWeb]::Track('spRuleHalfTitle0000018', 'Trixie Blues', @('Trixie Smith'), 'Blues', 193000),
	[FakeWeb]::Track('spRuleOnlyGuest0000019', 'Glow', @('Ama'), 'Glow', 200000),
	[FakeWeb]::Track('spRulePlainWins0000020', 'Shine', @('Ama'), 'Shine', 200000),
	[FakeWeb]::Track('spRuleOnlyLive00000021', 'Pulse', @('Ama'), 'Pulse', 200000),
	[FakeWeb]::Track('spRuleWithFeat00000022', 'Echo (with Zed)', @('Ama', 'Zed'), 'Echo', 200000),
	[FakeWeb]::Track('spRuleChannel000000023', 'Drift', @('Ama'), 'Drift', 200000),
	[FakeWeb]::Track('spRuleSameAlbum0000024', 'Tide', @('Ama'), 'Waves', 200000),
	[FakeWeb]::Track('spRuleOtherAlbum000025', 'Reef', @('Ama'), 'Waves', 200000),
	[FakeWeb]::Track('spRuleExplicit00000026', 'Burn', @('Ama'), 'Burn', 200000, $true)
)
$Web.YtmResults['Daniel Caesar H.E.R. Best Part'] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Song('9cHbvRUALrc', 'Pink + White', @('Frank Ocean'), 'Blonde', '3:05'),
	[FakeWeb]::Song('zNhtz0Lygik', 'Best Part (feat. H.E.R.)', @('Daniel Caesar', 'H.E.R.'), 'Freudian', '3:30')
)
$Web.YtmResults['Gnarls Barkley Crazy'] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Song('liveCrazy01', 'Crazy (Live at Coachella 2006)', @('Gnarls Barkley'), 'Live', '3:31'),
	[FakeWeb]::Song('studioCrazy', 'Crazy', @('Gnarls Barkley'), 'St. Elsewhere', '2:58')
)
$Web.YtmResults['The Late Hours Sway With Me (WCS Edit)'] = New-Object Collections.Generic.List[string]
[void]$Web.YtmBrokenQueries.Add('Nobody Offline')
$Web.YtmResults['KALEO Way Down We Go'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('trackAAAAA1', 'Way Down We Go', @('KALEO'), 'A/B', '3:34'))
$Web.YtmResults['Solo Only Here'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('trackDDDDD6', 'Only Here', @('Solo'), 'Here', '1:05'))
# Another id than the one PLslow1 lists
$Web.YtmResults['Turtle Slow One'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('searchSlow1', 'Slow One', @('Turtle'), 'Slow', '5:00'))
$Web.YtmResults['Tora Inundated'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('toraInundat', 'Inundated', @('ToraMusic'), 'A Force Majeure', '3:02'))
$Web.YtmResults['Feki Glass House Retrograde (Feki & Glasshouse Cover)'] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Song('retroGlass1', 'Retrograde with Glass House', @('Feki', 'Glass House'), 'Retrograde', '3:40')
)
$Web.YtmResults['Kenny Down Home Blues'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('bakersfield', 'Bakersfield Blues', @('Kenny'), 'Blues', '3:00'))
$Web.YtmResults['Bryce Fox StayLoose Horns (StayLoose Remix)'] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Song('hornsArcNth', 'Horns (Arc North Remix)', @('Arc North', 'Bryce Fox'), 'Horns', '3:24'),
	[FakeWeb]::Song('hornsStayLs', 'Horns (StayLoose Remix)', @('Bryce Fox', 'StayLoose'), 'Horns', '3:24')
)
$Web.YtmResults["Taylor Swift Everything Has Changed (Taylor's Version)"] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Song('changedOrig', 'Everything Has Changed', @('Taylor Swift'), 'Red', '4:05'),
	[FakeWeb]::Song('changedTays', "Everything Has Changed (Taylor's Version)", @('Taylor Swift'), 'Red', '4:05')
)
$Web.YtmResults['Lauv Love U Like That - Korean Version'] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Song('loveUOrigin', 'Love U Like That', @('Lauv'), 'Love U', '3:32'),
	[FakeWeb]::Song('loveUKorean', 'Love U Like That (Korean Version)', @('Lauv'), 'Love U', '3:32')
)
$Web.YtmResults['Tones And I Dance Monkey'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('tonesFive01', 'Dance Monkey', @('Tones Five'), 'Covers', '3:29'))
$Web.YtmResults['Lipps Inc. Funkytown'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('funkyTown01', 'Funky Town', @('Lipps Inc.'), 'Mouth', '4:00'))
$Web.YtmResults['Ke$ha Tik Tok'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('tikTokKesha', 'TiK ToK', @('Kesha'), 'Animal', '3:20'))
$Web.YtmResults['Trixie Smith Trixie Blues'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('prayingBlue', 'Praying Blues', @('Trixie Smith'), 'Blues', '3:14'))
$Web.YtmResults['Ama Glow'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('glowFeatZed', 'Glow (feat. Zed)', @('Ama', 'Zed'), 'Glow', '3:20'))
$Web.YtmResults['Ama Shine'] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Song('shineFeatZd', 'Shine (feat. Zed)', @('Ama', 'Zed'), 'Shine', '3:20'),
	[FakeWeb]::Song('shineSolo01', 'Shine', @('Ama'), 'Shine', '3:20')
)
$Web.YtmResults['Ama Pulse'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('pulseLive01', 'Pulse (Live)', @('Ama'), 'Live', '4:20'))
$Web.YtmResults['Ama Zed Echo (with Zed)'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('echoFeatZed', 'Echo (feat. Zed)', @('Ama'), 'Echo', '3:20'))
$Web.YtmResults['Ama Drift'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('driftMusic1', 'Drift', @('Ama Music'), 'Drift', '3:20'))
$Web.YtmResults['Ama Tide'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('tideAlbum01', 'Tide', @('Ama'), 'Waves', '3:27'))
$Web.YtmResults['Ama Reef'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('reefSingle1', 'Reef', @('Ama'), 'Reef', '3:27'))
$Web.YtmResults['Ama Burn'] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Song('burnClean01', 'Burn', @('Ama'), 'Burn', '3:20'),
	[FakeWeb]::Song('burnExplct1', 'Burn', @('Ama'), 'Burn', '3:20', $true)
)
$MatchFailures = 'spBrokenSearch00000004 search [HTTP 500 from 127.0.0.1] / spNoResults00000000003 noMatch [Searched "The Late Hours Sway With Me ' +
	'(WCS Edit)": no results | Searched "The Late Hours Sway With Me": no results] / spOtherTitle0000000007 noMatch [Searched "Kenny Down Home Blues": ' +
	'no usable results] / spRuleHalfTitle0000018 noMatch [Searched "Trixie Smith Trixie Blues": no usable results] / spRuleOnlyLive00000021 noMatch ' +
	'[Searched "Ama Pulse": no usable results] / spRuleOtherArtist00015 noMatch [Searched "Tones And I Dance Monkey": no usable results]'
$FailedQueries = 'The Late Hours Sway With Me (WCS Edit) / The Late Hours Sway With Me / Nobody Offline / Kenny Down Home Blues / ' +
	'Tones And I Dance Monkey / Trixie Smith Trixie Blues / Ama Pulse'

Install-FakeTools
Write-SyncPlaylists 'ytm:PLtracks1', "spotify:$MatchId"
Remove-Item "$StateFile.bak" -ErrorAction SilentlyContinue
$app = Start-App
$h = $app.Hwnd
$runs = Get-FetchRuns
$searches = @($Web.YtmQueries()).Count
Send-Command $h 'Sync &all'
Wait-Fetches $h $runs 1
# Way Down We Go and Cafe Lights are songs PLtracks1 lists, so they take its ids without a search
Check 'Sync all: the Spotify tracks searched, in playlist order' (Get-YtmQueriesSince $searches) ('Daniel Caesar H.E.R. Best Part / Gnarls Barkley Crazy / ' +
	'The Late Hours Sway With Me (WCS Edit) / The Late Hours Sway With Me / Nobody Offline / Solo Only Here / Kenny Down Home Blues / Turtle Slow One / ' +
	'Tora Inundated / Feki Glass House Retrograde (Feki & Glasshouse Cover) / Bryce Fox StayLoose Horns (StayLoose Remix) / ' +
	"Taylor Swift Everything Has Changed (Taylor's Version) / Lauv Love U Like That - Korean Version / Tones And I Dance Monkey / Lipps Inc. Funkytown / " +
	'Ke$ha Tik Tok / Trixie Smith Trixie Blues / Ama Glow / Ama Shine / Ama Pulse / Ama Zed Echo (with Zed) / Ama Drift / Ama Tide / Ama Reef / Ama Burn')
Check '  Tora as ToraMusic | the cover as Retrograde with Glass House' "$((Get-SavedState).sources.'spotify:spLetterArtist00000010'.match.ytmId) | $(
	@((Get-SavedMatches) -split ' / ' | Where-Object { $_ -like 'spWithTail*' }))" 'toraInundat | spWithTail000000000011 retroGlass1 1'
Check '  the remix by StayLoose, the re-recording, the Korean version, Funky Town, Kesha' (Get-SavedJudgements '^spRule(Dollar|Language|OwnVersion|Remixer|Spaced)') (
	'spRuleDollar0000000017 tikTokKesha 1 [] / spRuleLanguage00000014 loveUKorean 1 [] / spRuleOwnVersion000013 changedTays 1 [] / ' +
	'spRuleRemixer000000012 hornsStayLs 1 [] / spRuleSpacedTitle00016 funkyTown01 1 []')
Check '  the guest to review, the plain song, the credits, the channel, the albums, the explicit one' (
	Get-SavedJudgements '^spRule(OnlyGuest|PlainWins|WithFeat|Channel|SameAlbum|OtherAlbum|Explicit)') (
	'spRuleChannel000000023 driftMusic1 0.9 [] / spRuleExplicit00000026 burnExplct1 1 [] / spRuleOnlyGuest0000019 glowFeatZed 0.75 [version] / ' +
	'spRuleOtherAlbum000025 reefSingle1 0.75 [length] / spRulePlainWins0000020 shineSolo01 1 [] / spRuleSameAlbum0000024 tideAlbum01 0.9 [] / ' +
	'spRuleWithFeat00000022 echoFeatZed 1 []')
Check '  the explicit flags, album and length the match keeps' "$((Get-SavedState).sources.'spotify:spRuleExplicit00000026'.explicit) $(
	(Get-SavedState).sources.'spotify:spRuleExplicit00000026'.match.explicit) | $((Get-SavedState).sources.'spotify:spRuleSameAlbum0000024'.match.album) $(
	(Get-SavedState).sources.'spotify:spRuleSameAlbum0000024'.match.durationMs)" 'True True | Waves 207000'
Check '  matches: the best result, not the first; the studio version; the library''s' (@((Get-SavedMatches) -split ' / ' | Where-Object {
	$_ -notlike 'spLetterArtist*' -and $_ -notlike 'spWithTail*' -and $_ -notlike 'spRule*' }) -join ' / ') ('spClearMatch0000000001 zNhtz0Lygik 1 / ' +
	'spJoinedLater000000006 trackDDDDD6 1 / spLibraryFirst00000009 trackBBBBB2 1 / spLibraryLater00000008 searchSlow1 1 / ' +
	'spLiveLoses00000000002 studioCrazy 1 / spMergesYtm00000000005 trackAAAAA1 1')
Check '  failures: no results, a search that failed, another song, another artist, half the title' (Get-SavedFailures) $MatchFailures
$outcomes = @(Get-LogLines | Where-Object { $_ -match '^T (WARNING|ERROR) ' })
Check '  log: each no match in one line, without the searches' (@($outcomes | Where-Object { $_ -match '^T ERROR   No match found \[' }).Count) 5
Check '  log: the failed search' (@($outcomes | Where-Object { $_ -like '*spBrokenSearch*' }) -join ' / ') (
	"T ERROR   Couldn't search YouTube Music: HTTP 500 from 127.0.0.1 [Offline; track spotify:spBrokenSearch00000004]")
Check '  log: a review names its doubts' (@($outcomes | Where-Object { $_ -like '*spRuleOnlyGuest*' }) -join ' / ') (
	'T WARNING Needs review: "Glow (feat. Zed)" by Ama, Zed (75%), doubts: version [Glow; track spotify:spRuleOnlyGuest0000019]')
Check '  tracks: Way Down We Go and Cafe Lights on both are one track each' (Get-SavedTracks) ('spBrokenSearch00000004 failed [spBrokenSearch00000004] True / ' +
	'spClearMatch0000000001 pending [spClearMatch0000000001] True / spJoinedLater000000006 pending [spJoinedLater000000006] True / ' +
	'spLetterArtist00000010 pending [spLetterArtist00000010] True / spLibraryLater00000008 pending [spLibraryLater00000008] True / ' +
	'spLiveLoses00000000002 pending [spLiveLoses00000000002] True / spNoResults00000000003 failed [spNoResults00000000003] True / ' +
	'spOtherTitle0000000007 failed [spOtherTitle0000000007] True / spRuleChannel000000023 pending [spRuleChannel000000023] True / ' +
	'spRuleDollar0000000017 pending [spRuleDollar0000000017] True / spRuleExplicit00000026 pending [spRuleExplicit00000026] True / ' +
	'spRuleHalfTitle0000018 failed [spRuleHalfTitle0000018] True / spRuleLanguage00000014 pending [spRuleLanguage00000014] True / ' +
	'spRuleOnlyGuest0000019 pending [spRuleOnlyGuest0000019] True / spRuleOnlyLive00000021 failed [spRuleOnlyLive00000021] True / ' +
	'spRuleOtherAlbum000025 pending [spRuleOtherAlbum000025] True / spRuleOtherArtist00015 failed [spRuleOtherArtist00015] True / ' +
	'spRuleOwnVersion000013 pending [spRuleOwnVersion000013] True / spRulePlainWins0000020 pending [spRulePlainWins0000020] True / ' +
	'spRuleRemixer000000012 pending [spRuleRemixer000000012] True / spRuleSameAlbum0000024 pending [spRuleSameAlbum0000024] True / ' +
	'spRuleSpacedTitle00016 pending [spRuleSpacedTitle00016] True / spRuleWithFeat00000022 pending [spRuleWithFeat00000022] True / ' +
	'spWithTail000000000011 pending [spWithTail000000000011] True / AAAAA1 pending [AAAAA1,spMergesYtm00000000005] True / ' +
	'BBBBB2 pending [BBBBB2,spLibraryFirst00000009] True / CCCCC4 pending [CCCCC4] True / SHARE5 pending [SHARE5] True')
Check '  its row: one, both playlists, the album from Spotify' (@((Get-TrackRows $h) -split ' / ' | Where-Object { $_ -like '*Way Down We Go*' }) -join ' / ') (
	" | Way Down We Go | KALEO | A/B | $TracksName, Matches | 3:34")
Check '  no duplicate in All tracks: its Status tooltip' (Get-StatusTip $h 12) 'Waiting'
Check '  tabs | result line' (Get-TrackSummary $h) 'All 28 | Synced 0 | Waiting 22 | Needs review 2 | Failed 6 | Showing 28 of 28'

# A playlist with another id of Slow One: the Spotify track takes it and merges, the order of the playlists aside. The
# playlist with the id Only Here was matched to: its source joins that track. Only the failed tracks are searched
# again.
[void](Add-Playlist $h 'https://music.youtube.com/playlist?list=PLslow1')
[void](Add-Playlist $h 'https://music.youtube.com/playlist?list=PLshared1')
$runs = Get-FetchRuns
$searches = @($Web.YtmQueries()).Count
Send-Command $h 'Sync &all'
Wait-Fetches $h $runs 3
Check 'Sync all again: only the failed tracks searched' (Get-YtmQueriesSince $searches) $FailedQueries
Check '  Only Here: the YouTube Music source joined the Spotify track' (@((Get-SavedTracks) -split ' / ' | Where-Object { $_ -like 'spJoinedLater*' }) -join ' / ') (
	'spJoinedLater000000006 pending [spJoinedLater000000006,DDDDD6] True')
Check '  Slow One: the Spotify track merged into the playlist''s' "$(@((Get-SavedTracks) -split ' / ' | Where-Object { $_ -like '*Slow*' })) | $(
	@((Get-SavedMatches) -split ' / ' | Where-Object { $_ -like 'spLibraryLater*' }))" 'SLOWW7 pending [SLOWW7,spLibraryLater00000008] True | spLibraryLater00000008 trackSLOWW7 1'
Check '  failures as before' (Get-SavedFailures) $MatchFailures
Check '  the added playlist selected: Only Here among its rows' "$(Get-TrackRows $h) | $(Get-TrackSummary $h)" (
	"1 | $RowShared / 2 |  | Only Here | Solo | Here | Matches, $SharedName | 1:05 | " +
	"All 2 | Synced 0 | Waiting 2 | Needs review 0 | Failed 0 | Duplicates 0 | Showing 2 of 28 in $SharedName")
# Matches lists the track too, but that is another playlist: here it is no duplicate
Check '  Only Here there: no duplicate' (Get-StatusTip $h 1 1) 'Waiting'
Select-TrackRow $h 1
Check '  its card' (Wait-TrackCard $h 'ytm:trackDDDDD6') ("Track card:  / Only Here / Solo / Here / Status / Waiting / " +
	"Downloads on the next sync. / Sources / Listed on / YouTube Music /  / " +
	"Spotify / Playlists / Matches, $SharedName / Key / ytm:trackDDDDD6 / Audio / ID / trackDDDDD6 / Chosen by / Playlist / " +
	"Spotify match / Same ID (100%) / File / Not downloaded yet.")

# Cancel while a track is searched leaves it as it was
[void]$Web.YtmGate.Reset()
$searches = @($Web.YtmQueries()).Count
Send-Command $h 'Sync &all'
for ($i = 0; $i -lt 100 -and @($Web.YtmQueries()).Count -le $searches; $i++) { Start-Sleep -Milliseconds 100 }
Check 'A search held: syncing' (Test-Syncing $h) 'True'
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-matching.png") }
Send-Command $h '&Cancel'
for ($i = 0; $i -lt 50 -and (Test-Syncing $h); $i++) { Start-Sleep -Milliseconds 100 }
[void]$Web.YtmGate.Set()
Check '  Cancel: the run ends, the failures as they were' "$(Test-Syncing $h) | $(Get-SavedFailures)" "False | $MatchFailures"
Stop-App $app

"--- Run 7: matches to review"
# Two songs whose best result is 25 s off, another cut: one alone, one whose title is the longer one of a YouTube Music
# playlist's entry. Both need review, and the second stays apart from that entry until it is confirmed.
$ReviewId = '6reviewreviewreviewrev'
$Web.SpotifyNames[$ReviewId] = 'Reviews'
$Web.SpotifyItems[$ReviewId] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Track('spReviewAlone000000001', 'Tell Me', @('Rae'), 'Tell', 200000),
	[FakeWeb]::Track('spReviewApart000000002', 'Apart Song', @('Rae'), 'Apart', 200000)
)
$Web.YtmResults['Rae Tell Me'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('reviewVid01', 'Tell Me', @('Rae'), 'Tell', '3:45'))
$Web.YtmResults['Rae Apart Song'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('apartVid001', 'Apart Song Extended', @('Rae'), 'Apart', '3:45'))

# Each Spotify source with a match as "id ytmId confirmed"
function Get-SavedReviews {
	@((Get-SavedState).sources.PSObject.Properties | Where-Object { $_.Value.match } | ForEach-Object {
		"$($_.Name -replace $KeyPrefix, '') $($_.Value.match.ytmId) $([bool]$_.Value.match.confirmed)"
	}) -join ' / '
}

function Wait-SavedTracks([string]$like) {
	for ($i = 0; $i -lt 50 -and (Get-SavedTracks) -notlike $like; $i++) { Start-Sleep -Milliseconds 100 }
	Get-SavedTracks
}

$playlists = '"ytm:PLreview1": {"url": "https://music.youtube.com/playlist?list=PLreview1", "name": "Long ones", "addedAt": "2026-01-01T12:00:00Z", ' +
	'"sources": ["ytm:apartVid001"]}, ' +
	"`"spotify:$ReviewId`": {`"url`": `"https://open.spotify.com/playlist/$ReviewId`", `"name`": `"Reviews`", `"addedAt`": `"2026-01-02T12:00:00Z`"}"
$sources = '"ytm:apartVid001": {"title": "Apart Song Extended", "artists": ["Rae"], "durationMs": 225000, "track": "ytm:apartVid001"}'
$tracks = '"ytm:apartVid001": {"sources": ["ytm:apartVid001"], "status": "pending", "addedAt": "2026-01-01T12:00:00Z"}'
[IO.File]::WriteAllText($StateFile, "{`"version`": 1, `"playlists`": {$playlists}, `"sources`": {$sources}, `"tracks`": {$tracks}}")
Remove-Item "$StateFile.bak" -ErrorAction SilentlyContinue
$app = Start-App
$h = $app.Hwnd
# Until its tracks are fetched, Reviews has no counts line
$script:TallRows = @(1)
Click-PlaylistRow $h 2
$script:TallRows = 1, 2
$queries = Get-SpotifyQueries
Send-Command $h 'Sync &selected playlist'
Wait-Fetches $h $queries 1 -Spotify
Check 'Synced: both matched, neither confirmed' (Get-SavedReviews) 'spReviewAlone000000001 reviewVid01 False / spReviewApart000000002 apartVid001 False'
Check '  confidences and doubts: the cut, the cut with a longer title' (Get-SavedJudgements '^spReview') (
	'spReviewAlone000000001 reviewVid01 0.75 [length] / spReviewApart000000002 apartVid001 0.65 [title,length]')
Check '  the one with the entry''s id stays apart' (Get-SavedTracks) ('spReviewAlone000000001 pending [spReviewAlone000000001] True / ' +
	'spReviewApart000000002 pending [spReviewApart000000002] True / ytm:apartVid001 pending [ytm:apartVid001] True')
Check '  tabs | result line' (Get-TrackSummary $h) 'All 2 | Synced 0 | Waiting 2 | Needs review 2 | Failed 0 | Duplicates 0 | Showing 2 of 3 in Reviews'
Check '  Status tooltips' "$(Get-StatusTip $h 0 1) | $(Get-StatusTip $h 1 1)" 'Needs review | Needs review'
Check '  the playlist card counts them' ((Wait-CardTexts $h 'Playlist card') -like '* / To review / 2 / *') 'True'
Update-MenuStates $h
Check '  Confirm match grayed with nothing selected' ([W]::ItemsWithState([W]::GetMenu($h), 0x1) -like '*&Confirm match*') 'True'

Select-TrackRow $h 0
Check 'Tell Me: its doubt and Choose match' (Wait-TrackCard $h 'spotify:spReviewAlone000000001') ("Track card:  / Tell Me / Rae / Tell / " +
	"Status / Needs review / $Bullet 0:25 longer / $ChooseMatch / Source / Source / Spotify / " +
	"Playlists / Reviews / Key / spotify:spReviewAlone000000001 / Audio / Matched / Tell Me / Artists / Rae / Length / 3:45 (Spotify 3:20) / " +
	"ID / reviewVid01 / Chosen by / Matcher / Confidence / 75% / File / Not downloaded yet.")
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-review.png") }
$card = Get-InspectorCard $h
$doubts = @(Get-DirectChildren $card 'WestieLib.Label' | Where-Object { [W]::TextOf($_) -eq "$Bullet 0:25 longer" })[0]
# The card's title comes first
$matched = @(Get-DirectChildren $card 'WestieLib.Label' | Where-Object { [W]::TextOf($_) -eq 'Tell Me' })[-1]
Check '  in amber: the doubts, not the Audio rows' "$([W]::AmberInk($h, $doubts)) $([W]::AmberInk($h, $matched))" 'True False'
$choose = @([W]::Children((Get-InspectorCard $h), 'Button') | Where-Object { [W]::TextOf($_) -eq $ChooseMatch })[0]
Update-MenuStates $h
Check '  enabled: Choose match | grayed: menu''s Confirm match' "$([W]::IsWindowEnabled($choose)) | $([W]::ItemsWithState([W]::GetMenu($h), 0x1) -like '*&Confirm match*')" 'True | False'
$confidence = ([W]::Children((Get-InspectorCard $h), 'WestieLib.ConfidenceValue'))[0]
# 75% of the bar's 110 is 82.5
Check '  confidence bar: yellow to 75%, then empty' ([W]::PixelsIn($h, $confidence, @(0, $y, ((S 82) - 1), $y, ((S 82) + 2), $y)) -join ' ') 'd9a400 d9a400 e6e6e6'
Send-Command $h '&Confirm match'
for ($i = 0; $i -lt 50 -and (Get-SavedReviews) -notlike '*reviewVid01 True*'; $i++) { Start-Sleep -Milliseconds 100 }
Check 'Track > Confirm match: saved' (Get-SavedReviews) 'spReviewAlone000000001 reviewVid01 True / spReviewApart000000002 apartVid001 False'
Check '  tabs | its status' "$(Get-TrackSummary $h) | $(Get-StatusTip $h 0 1)" 'All 2 | Synced 0 | Waiting 2 | Needs review 1 | Failed 0 | Duplicates 0 | Showing 2 of 3 in Reviews | Waiting'
Check '  its card: Waiting, no button' ((Wait-TrackCard $h 'spotify:spReviewAlone000000001') -like '*Status / Waiting / Downloads on the next sync. / Source / *') 'True'

Select-TrackRow $h 1
Check 'Apart Song: a line for each doubt' (Wait-TrackCard $h 'spotify:spReviewApart000000002') ("Track card:  / Apart Song / Rae / Apart / " +
	"Status / Needs review / $Bullet Title differs slightly`n$Bullet 0:25 longer / $ChooseMatch / " +
	"Source / Source / Spotify / Playlists / Reviews / Key / spotify:spReviewApart000000002 / Audio / Matched / Apart Song Extended / " +
	"Artists / Rae / Length / 3:45 (Spotify 3:20) / ID / apartVid001 / Chosen by / Matcher / Confidence / 65% / File / Not downloaded yet.")
Send-Command $h '&Confirm match'
Check 'Confirm match: merged into the entry''s track' (Wait-SavedTracks '*apartVid001,spReview*') ('spReviewAlone000000001 pending [spReviewAlone000000001] True / ' +
	'ytm:apartVid001 pending [ytm:apartVid001,spReviewApart000000002] True')
Check '  saved confirmed' (Get-SavedReviews) 'spReviewAlone000000001 reviewVid01 True / spReviewApart000000002 apartVid001 True'
Check '  tabs' (Get-TrackSummary $h) 'All 2 | Synced 0 | Waiting 2 | Needs review 0 | Failed 0 | Duplicates 0 | Showing 2 of 2 in Reviews'

$searches = @($Web.YtmQueries()).Count
$queries = Get-SpotifyQueries
Send-Command $h 'Sync &selected playlist'
Wait-Fetches $h $queries 1 -Spotify
Check 'Synced again: matches aren''t searched again' "[$(Get-YtmQueriesSince $searches)] $(Get-SavedReviews)" (
	'[] spReviewAlone000000001 reviewVid01 True / spReviewApart000000002 apartVid001 True')
Stop-App $app

"--- Run 8: refused searches"
# A Spotify playlist with a song whose search YouTube Music refuses once, one whose search it keeps refusing, and three
# more. The refused ones wait while the others go ahead: the first matches on its second try, the second fails after its
# fourth.
$RefusedId = '7refusedrefusedrefused'
$Web.SpotifyNames[$RefusedId] = 'Refused'
$Web.SpotifyItems[$RefusedId] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Track('spRefusedOnce000000001', 'Once', @('Rae'), 'Once', 200000),
	[FakeWeb]::Track('spRefusedAlways0000002', 'Always', @('Rae'), 'Always', 200000),
	[FakeWeb]::Track('spRefusedAfter10000003', 'After One', @('Rae'), 'After', 200000),
	[FakeWeb]::Track('spRefusedAfter20000004', 'After Two', @('Rae'), 'After', 200000),
	[FakeWeb]::Track('spRefusedAfter30000005', 'After Three', @('Rae'), 'After', 200000)
)
$Web.YtmRefusals['Rae Once'] = 1
$Web.YtmRefusals['Rae Always'] = 99
Write-SyncPlaylists "spotify:$RefusedId"
Remove-Item "$StateFile.bak" -ErrorAction SilentlyContinue
$app = Start-App
$h = $app.Hwnd
$searches = @($Web.YtmQueries()).Count
$queries = Get-SpotifyQueries
$started = Get-Date
Send-Command $h 'Sync &all'
for ($i = 0; $i -lt 150 -and (Get-SpotifyQueries) -le $queries; $i++) { Start-Sleep -Milliseconds 100 }
# Each refused search waits at least 10 s, so the one refused four times takes over 30 s
for ($i = 0; $i -lt 600 -and (Test-Syncing $h); $i++) { Start-Sleep -Milliseconds 100 }
$seconds = ((Get-Date) - $started).TotalSeconds
Check 'Refused searches: the others go first, then the refused ones again' (Get-YtmQueriesSince $searches) (
	'Rae Once / Rae Always / Rae After One / Rae After Two / Rae After Three / Rae Once / Rae Always / Rae Always / Rae Always')
Check-That '  three waits of 10 s' "$([math]::Round($seconds)) s" ($seconds -ge 30 -and $seconds -lt 45) '30-45 s'
Check '  refused once: matched like the others' (Get-SavedMatches) ('spRefusedAfter10000003 ytspRefusedAfter10000003 1 / ' +
	'spRefusedAfter20000004 ytspRefusedAfter20000004 1 / spRefusedAfter30000005 ytspRefusedAfter30000005 1 / spRefusedOnce000000001 ytspRefusedOnce000000001 1')
Check '  refused four times: failed' (Get-SavedFailures) 'spRefusedAlways0000002 search [HTTP 403 from 127.0.0.1]'
Check '  log: one line for each, the tries at Debug' (@(Get-LogLines | Where-Object { $_ -match '^T (DEBUG|INFO|ERROR) .*track spotify:spRefused(Once|Always)' }) -join ' / ') (
	'T DEBUG   Searched "Rae Once": HTTP 403 from 127.0.0.1 [Once; track spotify:spRefusedOnce000000001] / ' +
	'T DEBUG   Searched "Rae Always": HTTP 403 from 127.0.0.1 [Always; track spotify:spRefusedAlways0000002] / ' +
	'T DEBUG   Searched "Rae Once": 1 result, best "Once" by Rae (100%) [Once; track spotify:spRefusedOnce000000001] / ' +
	'T INFO    Matched "Once" by Rae (100%) [Once; track spotify:spRefusedOnce000000001] / ' +
	'T DEBUG   Searched "Rae Always": HTTP 403 from 127.0.0.1 [Always; track spotify:spRefusedAlways0000002] / ' +
	'T DEBUG   Searched "Rae Always": HTTP 403 from 127.0.0.1 [Always; track spotify:spRefusedAlways0000002] / ' +
	'T DEBUG   Searched "Rae Always": HTTP 403 from 127.0.0.1 [Always; track spotify:spRefusedAlways0000002] / ' +
	"T ERROR   Couldn't search YouTube Music: HTTP 403 from 127.0.0.1 (tried 4 times) [Always; track spotify:spRefusedAlways0000002]")
Stop-App $app
