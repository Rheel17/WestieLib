# Downloading: an update downloads the tracks that have a YouTube Music id and don't need review, with the fake yt-dlp
# and ffmpeg. Each file lands in the library under its artists and title, tagged, with a cover from Spotify or the
# cropped thumbnail; a failed download keeps yt-dlp's error and is tried again; the format and the cover follow the
# settings, and a library folder Windows can't create fails the downloads. Files follow the state: a source joining a
# track re-tags its file, a track that leaves every playlist is Removed, and Keep and Delete file decide on it.
# Dot-sourced by ui-checks.ps1.

# A state.json with these playlists, never synced, added a day apart in this order
function Write-DownloadPlaylists([string[]]$keys) {
	$day = 0
	$entries = $keys | ForEach-Object {
		$day++
		$id = $_.Substring($_.IndexOf(':') + 1)
		$url = if ($_ -like 'spotify:*') { "https://open.spotify.com/playlist/$id" } else { "https://music.youtube.com/playlist?list=$id" }
		"`"$_`": {`"url`": `"$url`", `"name`": `"Named $id`", `"addedAt`": `"2026-01-$('{0:D2}' -f $day)T12:00:00Z`"}"
	}
	[IO.File]::WriteAllText($StateFile, '{"version": 1, "playlists": {' + ($entries -join ', ') + '}}')
}

# Starts Update all and waits for the update to end
function Invoke-UpdateAll($hwnd) {
	Send-Command $hwnd 'Update &all'
	for ($i = 0; $i -lt 20 -and -not (Test-Syncing $hwnd); $i++) { Start-Sleep -Milliseconds 100 }
	for ($i = 0; $i -lt 300 -and (Test-Syncing $hwnd); $i++) { Start-Sleep -Milliseconds 100 }
	Start-Sleep -Milliseconds 200
}

# Each track as "key status file ytmId", without the key's ytm: or spotify: prefix
function Get-SavedDownloads {
	@((Get-SavedState).tracks.PSObject.Properties | ForEach-Object {
		"$($_.Name -replace '^(ytm|spotify):', '') $($_.Value.status) [$($_.Value.file)] $($_.Value.ytmId)"
	}) -join ' / '
}

function Get-LibraryFiles { @(if (Test-Path $LibraryDir) { Get-ChildItem $LibraryDir -File | Sort-Object Name | ForEach-Object Name }) -join ' / ' }

# The file's Vorbis comments with these keys. Opus and FLAC files keep them uncompressed, each after its length in four
# little-endian bytes, so a comment is found by its key and read as long as the length says.
function Get-FileTags([string]$name, [string[]]$keys) {
	$bytes = [IO.File]::ReadAllBytes((Join-Path $LibraryDir $name))
	# One character per byte, so a place in the text is the same place in the bytes
	$text = [Text.Encoding]::GetEncoding(28591).GetString($bytes)
	@($keys | ForEach-Object {
		$key = "$_="
		for ($at = $text.IndexOf($key, [StringComparison]::Ordinal); $at -ge 4; $at = $text.IndexOf($key, $at + 1, [StringComparison]::Ordinal)) {
			$length = [BitConverter]::ToInt32($bytes, $at - 4)
			if ($length -ge $key.Length -and $at + $length -le $bytes.Length) { [Text.Encoding]::UTF8.GetString($bytes, $at, $length) }
		}
	}) -join ' / '
}

function Get-LogText { (Get-ChildItem (Join-Path $DataDir 'logs') -Filter '*.log' | Get-Content -Raw -Encoding UTF8) -join "`n" }

$DownloadSpotifyId = '3dowdowdowdowdowdowdow'
$Web.SpotifyNames[$DownloadSpotifyId] = 'Spotify Downloads'
$Web.SpotifyItems[$DownloadSpotifyId] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Track('spDownAccept000000001', 'Spot On', @('Ann'), 'Spot Album', 200000),
	[FakeWeb]::Track('spDownReview000000002', 'Maybe', @('Bea'), 'Unsure', 200000)
)
# Twenty seconds longer than Spotify's: a match with a length doubt, which needs review
$Web.YtmResults['Bea Maybe'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('reviewMaybe', 'Maybe', @('Bea'), 'Unsure', '3:40'))

"--- Run 1: an update downloads"
Set-SavedWindow @{}
Install-FakeTools
Set-SavedAutoUpdate $false
Write-DownloadPlaylists 'ytm:PLdownload1', "spotify:$DownloadSpotifyId"
$app = Start-App
$h = $app.Hwnd
Invoke-UpdateAll $h
Check 'Update all: each track downloaded once, none that needs review' ((Get-DownloadRuns | Sort-Object) -join ' ') 'badTRACK002 trackGOOD01 trackSLASH5 trackTWIN03 trackTWIN04 ytspDownAccept000000001'
Check '  tracks: file and id, a failure, a review left waiting' (Get-SavedDownloads) (
	'spDownAccept000000001 downloaded [Ann - Spot On.opus] ytspDownAccept000000001 / spDownReview000000002 pending []  / ' +
	'badTRACK002 failed []  / trackGOOD01 downloaded [Band - Good One.opus] trackGOOD01 / trackSLASH5 downloaded [Who - A_B_ Test_.opus] trackSLASH5 / ' +
	'trackTWIN03 downloaded [Pair - Twin.opus] trackTWIN03 / trackTWIN04 downloaded [Pair - Twin (2).opus] trackTWIN04')
$failure = (Get-SavedState).tracks.'ytm:badTRACK002'
$BotError = "ERROR: [youtube] badTRACK002: Sign in to confirm you$([char]0x2019)re not a bot"
Check '  the failure: yt-dlp''s error line, read from Windows-1252' "$($failure.failure) [$($failure.error)]" "download [$BotError]"
Check '  the library: the files, nothing else' (Get-LibraryFiles) 'Ann - Spot On.opus / Band - Good One.opus / Pair - Twin (2).opus / Pair - Twin.opus / Who - A_B_ Test_.opus'
Check '  the work folder emptied' @(Get-ChildItem (Join-Path $DataDir 'work') -ErrorAction SilentlyContinue).Count 0
$source = (Get-SavedState).sources.'ytm:trackGOOD01'
Check '  a YouTube Music source: album and year from the info JSON' "$($source.album) $($source.year)" 'Good Album 2021'
Check '  its tags' (Get-FileTags 'Band - Good One.opus' 'TITLE', 'ARTIST', 'ALBUM', 'DATE', 'WESTIELIB_SOURCE', 'WESTIELIB_YTMID') (
	'TITLE=Good One / ARTIST=Band / ALBUM=Good Album / DATE=2021 / WESTIELIB_SOURCE=ytm:trackGOOD01 / WESTIELIB_YTMID=trackGOOD01')
Check '  the Spotify track''s tags: its album' (Get-FileTags 'Ann - Spot On.opus' 'TITLE', 'ARTIST', 'ALBUM', 'WESTIELIB_SOURCE') (
	'TITLE=Spot On / ARTIST=Ann / ALBUM=Spot Album / WESTIELIB_SOURCE=spotify:spDownAccept000000001')
$covered = @('Band - Good One.opus', 'Ann - Spot On.opus' | ForEach-Object { (Get-FileTags $_ 'METADATA_BLOCK_PICTURE') -ne '' }) -join ' '
Check '  both with a cover' $covered 'True True'
Check '  the Spotify cover fetched from Spotify' ($Web.CoverRequests() -join ' ') 'spDownAccept000000001'
$crops = @(Get-Content (Join-Path $ToolsDir 'ffmpeg-runs.txt') -Encoding UTF8)
Check '  the thumbnails cropped square by ffmpeg' "$($crops.Count) $($crops[0] -match '\|-vf\|crop=min\(iw\\,ih\):min\(iw\\,ih\)\|')" '4 True'
$run = @(Get-Content $FakeYtDlpRuns -Encoding UTF8 | Where-Object { $_ -like '*v=trackGOOD01' })[0]
Check '  yt-dlp: Opus, deno from the tools folder' "$($run -match '\|--audio-format\|opus\|') $($run.Contains("|--js-runtimes|deno:$ToolsDir\deno.exe|"))" 'True True'
$log = Get-LogText
Check '  log: a download, a failure' "$($log -match 'INFO    Downloaded as "Band - Good One\.opus" \[Good One; track ytm:trackGOOD01\]') $($log.Contains("ERROR   Couldn't download: $BotError"))" 'True True'

$downloads = @(Get-DownloadRuns).Count
Invoke-UpdateAll $h
Check 'Update all again: only the failed track tried again' (@(Get-DownloadRuns | Select-Object -Skip $downloads) -join ' ') 'badTRACK002'
Stop-App $app

"--- Run 2: FLAC, no cover"
Set-SavedSetting format flac
Set-SavedSetting embedCover $false
Write-DownloadPlaylists 'ytm:PLflac1'
Remove-Item (Join-Path $ToolsDir 'ffmpeg-runs.txt') -ErrorAction SilentlyContinue
$Web.ForgetQueries()
$app = Start-App
$h = $app.Hwnd
Invoke-UpdateAll $h
Check 'Update all: a FLAC file' (Get-SavedDownloads) 'trackFLAC06 downloaded [Flac - Lossless.flac] trackFLAC06'
Check '  its tags' (Get-FileTags 'Flac - Lossless.flac' 'TITLE', 'ARTIST', 'WESTIELIB_YTMID') 'TITLE=Lossless / ARTIST=Flac / WESTIELIB_YTMID=trackFLAC06'
Check '  no cover: no crop, no picture' "$(Test-Path (Join-Path $ToolsDir 'ffmpeg-runs.txt')) $((Get-FileTags 'Flac - Lossless.flac' 'METADATA_BLOCK_PICTURE') -eq '')" 'False True'
Stop-App $app

"--- Run 3: the download in the window"
Set-SavedSetting format opus
Set-SavedSetting embedCover $true
Write-DownloadPlaylists 'ytm:PLretry1', 'ytm:PLhold1'
[IO.File]::WriteAllText((Join-Path $ToolsDir 'fail-ids.txt'), "trackRETRY7`n")
$hold = Join-Path $ToolsDir 'hold-trackHOLD08'
[IO.File]::WriteAllText($hold, '')
$app = Start-App
$h = $app.Hwnd
Send-Command $h 'Update &all'
for ($i = 0; $i -lt 100 -and (Get-DownloadRuns) -notcontains 'trackHOLD08'; $i++) { Start-Sleep -Milliseconds 100 }
Start-Sleep -Milliseconds 600
# By title: Again, then Held
Check 'A download held at 50%: its Status tooltip | syncing' "$(Get-StatusTip $h 1) | $(Test-Syncing $h)" 'Downloading 50% | True'
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-downloading.png") }
Remove-Item $hold
for ($i = 0; $i -lt 100 -and (Test-Syncing $h); $i++) { Start-Sleep -Milliseconds 100 }
Start-Sleep -Milliseconds 200
Check '  let go: downloaded, the other failed' (Get-SavedDownloads) 'trackHOLD08 downloaded [Slowpoke - Held.opus] trackHOLD08 / trackRETRY7 failed [] '
Check '  its Status tooltip' (Get-StatusTip $h 1) 'Downloaded'

Select-TrackRow $h 1
$card = Wait-TrackCard $h 'ytm:trackHOLD08'
Check 'Held selected: its card''s status and file' ($card -replace '^.* / Status / ', 'Status / ' -replace ' / Source / .* / File / ', ' / File / ') (
	'Status / Downloaded / The file is in the library and tagged. / File / Name / Slowpoke - Held.opus / Show in folder')
$cover = ([W]::Children((Get-InspectorCard $h), 'WestieLib.CoverArt'))[0]
$pixel = [W]::PixelsIn($h, $cover, @((S 72), (S 72)))[0]
# The fixture's thumbnail is all #3366CC; JPEG moves a channel a little
$near = @(0, 2, 4 | ForEach-Object { [math]::Abs([Convert]::ToInt32($pixel.Substring($_, 2), 16) - [Convert]::ToInt32('3366cc'.Substring($_, 2), 16)) -le 12 }) -notcontains $false
Check-That '  the cover from the file' $pixel $near 'near 3366cc'
Update-MenuStates $h
$grayed = [W]::ItemsWithState([W]::GetMenu($h), 0x1)
Check '  Track menu: Retry grayed, Show in folder not' "$($grayed -like '*&Retry*') $($grayed -like '*&Show in folder*')" 'True False'

Select-TrackRow $h 0
$card = Wait-TrackCard $h 'ytm:trackRETRY7'
Update-MenuStates $h
$grayed = [W]::ItemsWithState([W]::GetMenu($h), 0x1)
Check 'Again selected: Retry on its card | in the menu | Show in folder grayed' (
	"$($card -like '* / Couldn''t download / ERROR: * / Retry / Source / *') | $($grayed -notlike '*&Retry*') | $($grayed -like '*&Show in folder*')") 'True | True | True'
Remove-Item (Join-Path $ToolsDir 'fail-ids.txt')
$downloads = @(Get-DownloadRuns).Count
Send-Command $h '&Retry'
for ($i = 0; $i -lt 20 -and -not (Test-Syncing $h) -and (Get-SavedDownloads) -like '*trackRETRY7 failed*'; $i++) { Start-Sleep -Milliseconds 100 }
for ($i = 0; $i -lt 100 -and (Test-Syncing $h); $i++) { Start-Sleep -Milliseconds 100 }
Start-Sleep -Milliseconds 200
Check 'Track > Retry: only that track downloaded again' "$(@(Get-DownloadRuns | Select-Object -Skip $downloads) -join ' ') | $(Get-SavedDownloads)" (
	'trackRETRY7 | trackHOLD08 downloaded [Slowpoke - Held.opus] trackHOLD08 / trackRETRY7 downloaded [Retrier - Again.opus] trackRETRY7')
Check '  log' ((Get-LogText) -match 'INFO    Retrying 1 track \[Again; track ytm:trackRETRY7\]') 'True'

# The Files and Downloads groups show the settings, and take a change on OK
Send-Command $h '&Settings...'
$dialog = Wait-OwnedWindow $h
$checked = @(1007, 1008, 1009 | ForEach-Object { [W]::SendMessageW([W]::GetDlgItem($dialog, $_), 0xF0, [IntPtr]::Zero, [IntPtr]::Zero) }) -join ' '
Check 'Settings: Opus, embed, 2 parallel, no pause' "$checked | $([W]::EditText([W]::GetDlgItem($dialog, 1010))) $([W]::EditText([W]::GetDlgItem($dialog, 1012)))" '1 0 1 | 2 0'
if ($Screenshot) { Save-Screenshot $dialog ("$($Screenshot -replace '\.png$', '')-settings-downloads.png") }
[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1008), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1009), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
[W]::SetEditText([W]::GetDlgItem($dialog, 1010), '9')
[W]::SetEditText([W]::GetDlgItem($dialog, 1012), '5')
Start-Sleep -Milliseconds 200
[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Wait-Closed $dialog
$saved = Get-Content $SettingsFile -Raw -Encoding UTF8 | ConvertFrom-Json
Check '  FLAC, no cover, 9 held to 4, a pause of 5: saved' "$($saved.format) $($saved.embedCover) $($saved.concurrency) $($saved.delayBetweenTracksSeconds)" 'flac False 4 5'
Stop-App $app

"--- Run 4: a library folder Windows can't create"
Set-SavedSetting libraryPath (Join-Path $LibraryDir 'CON')
Write-DownloadPlaylists 'ytm:PLflac2'
$app = Start-App
$h = $app.Hwnd
Invoke-UpdateAll $h
$failure = (Get-SavedState).tracks.'ytm:trackFLAC06'
Check 'Update all: the download fails' "$($failure.status) [$($failure.error)]" 'failed [Windows can''t use "CON" as a folder name in the library folder]'
Stop-App $app

"--- Run 5: files follow the tracks"
Set-SavedSetting libraryPath $LibraryDir
Set-SavedSetting format opus
Set-SavedSetting embedCover $true
Remove-Item (Join-Path $LibraryDir '*') -Force
$MergeSpotifyId = '3mermermermermermermer'
$Web.SpotifyNames[$MergeSpotifyId] = 'Spotify Merges'
$Web.SpotifyItems[$MergeSpotifyId] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Track('spMergeGood0000000001', 'Good One', @('Band'), 'Good Album', 200000),
	[FakeWeb]::Track('spGoneSoon00000000001', 'Gone Soon', @('Cee'), 'Leaving', 180000)
)
$Web.YtmResults['Band Good One'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('trackGOOD01', 'Good One', @('Band'), 'Good Album', '3:20'))
$Web.YtmResults['Cee Gone Soon'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('trackGONE09', 'Gone Soon', @('Cee'), 'Leaving', '3:00'))
Write-DownloadPlaylists "spotify:$MergeSpotifyId"
$app = Start-App
$h = $app.Hwnd
Invoke-UpdateAll $h
Check 'Update all: both matched and downloaded' (Get-SavedDownloads) (
	'spGoneSoon00000000001 downloaded [Cee - Gone Soon.opus] trackGONE09 / spMergeGood0000000001 downloaded [Band - Good One.opus] trackGOOD01')
Stop-App $app

# A YouTube Music playlist lists the id the Spotify track matched, so its source joins that track and the file's tags
$state = [IO.File]::ReadAllText($StateFile)
$entry = '"ytm:PLgood1": {"url": "https://music.youtube.com/playlist?list=PLgood1", "name": "Named PLgood1", "addedAt": "2026-01-05T12:00:00Z"}, '
[IO.File]::WriteAllText($StateFile, ([regex]'"playlists"\s*:\s*\{').Replace($state, "`$0$entry", 1))
$downloads = @(Get-DownloadRuns).Count
$app = Start-App
$h = $app.Hwnd
Invoke-UpdateAll $h
$good = (Get-SavedState).tracks.'spotify:spMergeGood0000000001'
Check 'A YouTube Music playlist lists it: one track, both sources' "$($good.sources -join ' ') | $((Get-SavedState).tracks.PSObject.Properties.Name -contains 'ytm:trackGOOD01')" (
	'spotify:spMergeGood0000000001 ytm:trackGOOD01 | False')
Check '  no download, the same file' "$(@(Get-DownloadRuns | Select-Object -Skip $downloads).Count) | $(Get-LibraryFiles)" '0 | Band - Good One.opus / Cee - Gone Soon.opus'
Check '  the file re-tagged with both' (Get-FileTags 'Band - Good One.opus' 'TITLE', 'WESTIELIB_SOURCE', 'WESTIELIB_YTMID') (
	'TITLE=Good One / WESTIELIB_SOURCE=spotify:spMergeGood0000000001 / WESTIELIB_SOURCE=ytm:trackGOOD01 / WESTIELIB_YTMID=trackGOOD01')
Check '  its cover kept' ((Get-FileTags 'Band - Good One.opus' 'METADATA_BLOCK_PICTURE') -ne '') 'True'

# Whether the infobar shows, and its message
function Get-NoticeText($hwnd) {
	$infobar = Get-Infobar $hwnd
	"$([W]::IsWindowVisible($infobar)) $([W]::TextOf(([W]::Children($infobar, 'WestieLib.Label'))[0]))"
}

$Web.SpotifyItems[$MergeSpotifyId].RemoveAt(1)
Invoke-UpdateAll $h
Check 'Gone Soon leaves the playlist: Removed, the file stays' "$((Get-SavedState).tracks.'spotify:spGoneSoon00000000001'.status) | $(Get-LibraryFiles)" (
	'orphaned | Band - Good One.opus / Cee - Gone Soon.opus')
Check '  the infobar' (Get-NoticeText $h) 'True 1 track is no longer in any playlist.'
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-removed.png") }
[void][W]::PostMessageW((Get-InfobarButton $h), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Start-Sleep -Milliseconds 300
$rows = @((Get-TrackRows $h) -split ' / ')
Check '  Review: the Removed tab, only that track' "$($rows.Count) $($rows[0] -like '*Gone Soon*')" '1 True'

Select-TrackRow $h 0
$card = Wait-TrackCard $h 'spotify:spGoneSoon00000000001'
Update-MenuStates $h
$grayed = [W]::ItemsWithState([W]::GetMenu($h), 0x1)
Check 'Gone Soon selected: Keep and Delete file on its card | in the menu' (
	"$($card -like "* / Removed / No longer in any playlist. The file is still in the library. / Keep / Delete file$Ellipsis / Source / *") | $($grayed -notlike '*&Keep*') $($grayed -notlike '*&Delete file*')") 'True | True True'
Send-Command $h '&Keep'
Start-Sleep -Milliseconds 300
Check 'Track > Keep: Kept, the file stays, the infobar goes' "$((Get-SavedState).tracks.'spotify:spGoneSoon00000000001'.status) | $(Get-LibraryFiles) | $(Get-NoticeText $h)" (
	'kept | Band - Good One.opus / Cee - Gone Soon.opus | False 1 track is no longer in any playlist.')
Check '  log' ((Get-LogText) -match 'INFO    Kept the file \[Gone Soon; track spotify:spGoneSoon00000000001\]') 'True'

Click-Tab $h 0
Start-Sleep -Milliseconds 200
$rows = @((Get-TrackRows $h) -split ' / ')
Select-TrackRow $h ([array]::FindIndex($rows, [Predicate[string]]{ param($row) $row -like '*Gone Soon*' }))
$card = Wait-TrackCard $h 'spotify:spGoneSoon00000000001'
Check 'Kept selected: Delete file on its card' ($card -like "* / Kept / No longer in any playlist, but kept. / Delete file$Ellipsis / Source / *") 'True'
Send-Command $h '&Delete file...'
$dialog = Wait-OwnedWindow $h
Check '  Track > Delete file: the confirmation' ([W]::TextOf($dialog)) 'Delete file'
if ($Screenshot) { Save-Screenshot $dialog ("$($Screenshot -replace '\.png$', '')-delete-file.png") }
# TDM_CLICK_BUTTON with IDOK, the Delete button
[void][W]::PostMessageW($dialog, 0x466, [IntPtr]1, [IntPtr]::Zero)
Wait-Closed $dialog
Start-Sleep -Milliseconds 300
$saved = Get-SavedState
Check '  Delete: the track and its source gone, and the file' (
	"$($saved.tracks.PSObject.Properties.Name -contains 'spotify:spGoneSoon00000000001') $($saved.sources.PSObject.Properties.Name -contains 'spotify:spGoneSoon00000000001') | $(Get-LibraryFiles)") (
	'False False | Band - Good One.opus')
Check '  log' ((Get-LogText) -match 'INFO    Deleted "Cee - Gone Soon\.opus" \[Gone Soon; track spotify:spGoneSoon00000000001\]') 'True'
Stop-App $app