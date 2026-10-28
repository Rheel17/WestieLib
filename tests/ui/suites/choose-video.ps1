# The Choose a video dialog: opening it, the candidates it lists (the current match, the library's YouTube Music
# entries and the search results, by score), searching again, pasted links, and what Use this video does: confirm the
# current match, merge into the library's track, set an override, or split an entry out of a merged track.
# Dot-sourced by ui-checks.ps1.

$PicksId = '8pickspickspickspicksp'
$TellKey = 'spotify:spTellMe00000000000001'
$LostKey = 'spotify:spNoMatch0000000000002'
$MergedKey = 'spotify:spMerged00000000000003'

# A track as "status [sources] override failure", or "-" when there is none
function Get-SavedTrack([string]$key) {
	$track = (Get-SavedState).tracks.$key
	if (-not $track) { return '-' }
	"$($track.status) [$($track.sources -join ',')] $($track.videoOverride) $($track.failure)"
}

function Wait-SavedTrack([string]$key, [string]$like) {
	for ($i = 0; $i -lt 50 -and (Get-SavedTrack $key) -notlike $like; $i++) { Start-Sleep -Milliseconds 100 }
	Get-SavedTrack $key
}

# Selects row $index (from 0) of a list with Home and Down, as Select-TrackRow does in the sync suite
function Select-ListRow($list, [int]$index) {
	[void][W]::SendMessageW($list, 0x100, [IntPtr]0x24, [IntPtr]1)
	for ($i = 0; $i -lt $index; $i++) { [void][W]::SendMessageW($list, 0x100, [IntPtr]0x28, [IntPtr]1) }
}

function Click-CardButton($hwnd, [string]$text) {
	$button = @([W]::Children((Get-InspectorCard $hwnd), 'Button') | Where-Object { [W]::TextOf($_) -eq $text })[0]
	[void][W]::PostMessageW($button, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
}

function Get-VideoList($dialog) { [W]::GetDlgItem($dialog, 1204) }

# The list's rows, then what it reads while it has none (its window text)
function Get-VideoRows($dialog) {
	$list = Get-VideoList $dialog
	$count = [int][W]::SendMessageW($list, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero)
	$rows = @(if ($count -gt 0) { 0..($count - 1) | ForEach-Object { [W]::ListViewRow($list, $_) } }) -join ' / '
	"[$rows] $([W]::TextOf($list))"
}

# Waits for the searches the dialog started after $searched searches had reached FakeWeb, then for their answer
function Wait-VideoRows($dialog, [int]$searched, [int]$count = 1) {
	for ($i = 0; $i -lt 50 -and @($Web.YtmQueries()).Count -lt $searched + $count; $i++) { Start-Sleep -Milliseconds 100 }
	for ($i = 0; $i -lt 50 -and [W]::TextOf((Get-VideoList $dialog)) -like 'Searching*'; $i++) { Start-Sleep -Milliseconds 100 }
	Get-VideoRows $dialog
}

# The link's detection line, then whether Open in browser and Use this video are enabled
function Get-ChoiceState($dialog) {
	"$([W]::TextOf([W]::GetDlgItem($dialog, 1206))) | $([W]::IsWindowEnabled([W]::GetDlgItem($dialog, 1207))) $([W]::IsWindowEnabled([W]::GetDlgItem($dialog, 1)))"
}

function Set-VideoLink($dialog, [string]$link) { [W]::SetEditText([W]::GetDlgItem($dialog, 1205), $link) }

function Use-ThisVideo($dialog) {
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
}

function Open-ChooseVideo($hwnd, [scriptblock]$open) {
	$searched = @($Web.YtmQueries()).Count
	& $open
	$dialog = Wait-OwnedWindow $hwnd
	[pscustomobject]@{ Dialog = $dialog; Before = $searched }
}

# A YouTube Music playlist with Tell Me, another song and the video of a merged track, and a Spotify playlist: Tell Me
# matched to a live version (78%, to review), a song without a match, and one merged into the YouTube Music track
$playlists = '"ytm:PLlib1": {"url": "https://music.youtube.com/playlist?list=PLlib1", "name": "Library list", "addedAt": "2026-01-01T12:00:00Z", ' +
	'"sources": ["ytm:libSong0001", "ytm:otherSong01", "ytm:sharedVid01"]}, ' +
	"`"spotify:$PicksId`": {`"url`": `"https://open.spotify.com/playlist/$PicksId`", `"name`": `"Picks`", `"addedAt`": `"2026-01-02T12:00:00Z`", " +
	"`"sources`": [`"$TellKey`", `"$LostKey`", `"$MergedKey`"]}"
$sources = @(
	'"ytm:libSong0001": {"title": "Tell Me", "artists": ["Rae"], "durationMs": 201000, "track": "ytm:libSong0001"}',
	'"ytm:otherSong01": {"title": "Something Else", "artists": ["Nobody"], "durationMs": 100000, "track": "ytm:otherSong01"}',
	'"ytm:sharedVid01": {"title": "Shared Song", "artists": ["Both"], "durationMs": 200000, "track": "ytm:sharedVid01"}',
	("`"$TellKey`": {`"title`": `"Tell Me`", `"artists`": [`"Rae`"], `"album`": `"Tell`", `"durationMs`": 200000, " +
		'"match": {"videoId": "reviewVid01", "confidence": 0.78, "title": "Tell Me (Live)", "artists": ["Rae"], "durationMs": 260000}, ' +
		"`"track`": `"$TellKey`"}"),
	"`"$LostKey`": {`"title`": `"Lost Song`", `"artists`": [`"Gone`"], `"album`": `"Away`", `"durationMs`": 180000, `"track`": `"$LostKey`"}",
	("`"$MergedKey`": {`"title`": `"Shared Song`", `"artists`": [`"Both`"], `"album`": `"Shared`", `"durationMs`": 200000, " +
		'"match": {"videoId": "sharedVid01", "confidence": 0.95, "title": "Shared Song", "artists": ["Both"], "durationMs": 200000}, "track": "ytm:sharedVid01"}')
)
$tracks = @(
	'"ytm:libSong0001": {"sources": ["ytm:libSong0001"], "status": "pending", "addedAt": "2026-01-01T12:00:00Z"}',
	'"ytm:otherSong01": {"sources": ["ytm:otherSong01"], "status": "pending", "addedAt": "2026-01-01T12:00:00Z"}',
	"`"ytm:sharedVid01`": {`"sources`": [`"ytm:sharedVid01`", `"$MergedKey`"], `"status`": `"pending`", `"addedAt`": `"2026-01-01T12:00:00Z`"}",
	"`"$TellKey`": {`"sources`": [`"$TellKey`"], `"status`": `"pending`", `"addedAt`": `"2026-01-02T12:00:00Z`"}",
	("`"$LostKey`": {`"sources`": [`"$LostKey`"], `"status`": `"failed`", `"failure`": `"noMatch`", " +
		"`"error`": `"Searched \`"Gone Lost Song\`": no results`", `"addedAt`": `"2026-01-02T12:00:00Z`"}")
)
$Web.YtmResults['Rae Tell Me'] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Song('searchTell1', 'Tell Me', @('Rae'), 'Tell', '3:20'),
	[FakeWeb]::Song('reviewVid01', 'Tell Me (Live)', @('Rae'), 'Live', '4:20'),
	[FakeWeb]::Song('karaoke0001', 'Tell Me (Karaoke)', @('Sing'), 'Karaoke', '3:30')
)
$Web.YtmResults['Tell Me Rae'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('otherTell01', 'Tell Me', @('Rae'), 'Other', '3:22'))
$Web.YtmResults['Gone Lost Song'] = New-Object Collections.Generic.List[string]
[void]$Web.YtmBrokenQueries.Add('Broken query')

"--- Run 1: choosing videos"
[IO.File]::WriteAllText($StateFile, "{`"version`": 1, `"playlists`": {$playlists}, `"sources`": {$($sources -join ', ')}, `"tracks`": {$($tracks -join ', ')}}")
$app = Start-App
$h = $app.Hwnd
$script:TallRows = 1, 2
Click-PlaylistRow $h 2
$table = Get-TrackTable $h

Select-ListRow $table 0
Check 'Tell Me, to review: the card offers Choose video' ((Wait-TrackCard $h $TellKey) -like "* / Needs review / * / $ChooseVideo / Source / *") 'True'
Update-MenuStates $h
$grayed = [W]::ItemsWithState([W]::GetMenu($h), 0x1)
Check '  menu: Confirm match and Choose video enabled' "$($grayed -like '*&Confirm match*') $($grayed -like '*C&hoose video*')" 'False False'
$open = Open-ChooseVideo $h { Click-CardButton $h $ChooseVideo }
$dialog = $open.Dialog
Check '  the card''s button opens it' "$([W]::ClassOf($dialog)) $([W]::TextOf($dialog))" '#32770 Choose a video'
Check '  the track | the current video | the query' (
	"$([W]::TextOf([W]::GetDlgItem($dialog, 1200))) | $([W]::TextOf([W]::GetDlgItem($dialog, 1201))) | $([W]::EditText([W]::GetDlgItem($dialog, 1202)))") (
	"Tell Me $MiddleDot Rae $MiddleDot Tell $MiddleDot 3:20 | Current match: Tell Me (Live), 78% confidence | Rae Tell Me")
Check '  the Matcher''s query searched: the library''s song, the results and the current match, by score' (Wait-VideoRows $dialog $open.Before) (
	'[Tell Me (in library) | Rae |  | 3:21 | 100% / Tell Me | Rae | Tell | 3:20 | 100% / Tell Me (Live) (current) | Rae |  | 4:20 | 40% / ' +
	"Tell Me (Karaoke) | Sing | Karaoke | 3:30 | 30%] No results. Try fewer words, or paste a link below.")
if ($Screenshot) { Save-Screenshot $dialog ("$($Screenshot -replace '\.png$', '')-choose-video.png") }
Check '  nothing chosen yet: no line, buttons disabled' (Get-ChoiceState $dialog) ' | False False'
Select-ListRow (Get-VideoList $dialog) 1
Check '  a row selected: buttons enabled' (Get-ChoiceState $dialog) ' | True True'
$links = @(
	@{ Link = 'hello'; Line = 'Not a YouTube link | True True' },
	@{ Link = ' https://youtu.be/libSong0001?si=x '; Line = "YouTube video $MiddleDot libSong0001 Tell Me | True True" },
	@{ Link = 'MUSIC.youtube.com/watch?list=x&v=abcdefghijk'; Line = "YouTube video $MiddleDot abcdefghijk | True True" },
	@{ Link = 'https://m.youtube.com/shorts/abcdefghijk'; Line = "YouTube video $MiddleDot abcdefghijk | True True" },
	@{ Link = 'abcdefghij'; Line = 'Not a YouTube link | True True' },
	@{ Link = ''; Line = ' | True True' }
)
foreach ($case in $links) {
	Set-VideoLink $dialog $case.Link
	Check "  link '$($case.Link)'" (Get-ChoiceState $dialog) $case.Line
}

$searched = @($Web.YtmQueries()).Count
[W]::SetEditText([W]::GetDlgItem($dialog, 1202), 'Tell Me Rae')
[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1203), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Check 'Search: its results, the current match and the library''s song stay' (Wait-VideoRows $dialog $searched) (
	'[Tell Me (in library) | Rae |  | 3:21 | 100% / Tell Me | Rae | Other | 3:22 | 100% / Tell Me (Live) (current) | Rae |  | 4:20 | 40%] ' +
	'No results. Try fewer words, or paste a link below.')
$searched = @($Web.YtmQueries()).Count
[W]::SetEditText([W]::GetDlgItem($dialog, 1202), 'Broken query')
[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1203), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Check '  a search that fails' (Wait-VideoRows $dialog $searched) (
	'[Tell Me (in library) | Rae |  | 3:21 | 100% / Tell Me (Live) (current) | Rae |  | 4:20 | 40%] Couldn''t search YouTube Music: HTTP 500 from 127.0.0.1')
Select-ListRow (Get-VideoList $dialog) 1
Use-ThisVideo $dialog
for ($i = 0; $i -lt 50 -and -not (Get-SavedState).sources.$TellKey.match.confirmed; $i++) { Start-Sleep -Milliseconds 100 }
Check 'Use this video on the current match: confirmed, nothing else' "$(Get-SavedTrack $TellKey) | $((Get-SavedState).sources.$TellKey.match.confirmed)" (
	"pending [$TellKey]   | True")
Check '  Waiting now' ((Wait-TrackCard $h $TellKey) -like '* / Status / Waiting / *') 'True'

$open = Open-ChooseVideo $h { Send-Command $h 'C&hoose video...' }
$dialog = $open.Dialog
[void](Wait-VideoRows $dialog $open.Before)
Select-ListRow (Get-VideoList $dialog) 0
Use-ThisVideo $dialog
Check 'Track > Choose video, the library''s song: merged into its track' "$(Wait-SavedTrack 'ytm:libSong0001' '*,spotify*') | $(Get-SavedTrack $TellKey)" (
	"pending [ytm:libSong0001,$TellKey]   | -")

Select-ListRow $table 1
Check 'Lost Song, no match: the card offers Choose video' ((Wait-TrackCard $h $LostKey) -like "* / No usable video was found. Choose one yourself. / * / $ChooseVideo / Source / *") 'True'
$open = Open-ChooseVideo $h { Send-Command $h 'C&hoose video...' }
$dialog = $open.Dialog
Check '  no current video, no results' "$([W]::TextOf([W]::GetDlgItem($dialog, 1201))) | $(Wait-VideoRows $dialog $open.Before)" (
	'The Matcher found no usable video. | [] No results. Try fewer words, or paste a link below.')
Set-VideoLink $dialog 'https://www.youtube.com/watch?v=customVid01'
Use-ThisVideo $dialog
Check '  a pasted link: the override, no failure' (Wait-SavedTrack $LostKey '*customVid01*') "pending [$LostKey] customVid01 "
Check '  its card' ((Wait-TrackCard $h $LostKey) -like "* / Status / Waiting / * / ID / customVid01 / Chosen by / Override / File / *") 'True'

Select-ListRow $table 2
[void](Wait-TrackCard $h $MergedKey)
$open = Open-ChooseVideo $h { Send-Command $h 'C&hoose video...' }
$dialog = $open.Dialog
Check 'Shared Song, merged: the current video is the playlist''s' "$([W]::TextOf([W]::GetDlgItem($dialog, 1201))) | $(Wait-VideoRows $dialog $open.Before)" (
	'Current video: Shared Song, from a YouTube Music playlist | [Shared Song (current) | Both |  | 3:20 | 100%] No results. Try fewer words, or paste a link below.')
Set-VideoLink $dialog 'splitVid001'
Use-ThisVideo $dialog
Check '  another video: the entry splits out with it' "$(Wait-SavedTrack $MergedKey '*splitVid001*') | $(Get-SavedTrack 'ytm:sharedVid01')" (
	"pending [$MergedKey] splitVid001  | pending [ytm:sharedVid01]  ")

# The tracks with a video of the user's aren't matched again
$Web.SpotifyNames[$PicksId] = 'Picks'
$Web.SpotifyItems[$PicksId] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Track('spTellMe00000000000001', 'Tell Me', @('Rae'), 'Tell', 200000),
	[FakeWeb]::Track('spNoMatch0000000000002', 'Lost Song', @('Gone'), 'Away', 180000),
	[FakeWeb]::Track('spMerged00000000000003', 'Shared Song', @('Both'), 'Shared', 200000)
)
$searches = @($Web.YtmQueries()).Count
$spotify = @($Web.SpotifyQueries()).Count
Send-Command $h 'Sync &selected playlist'
for ($i = 0; $i -lt 50 -and @($Web.SpotifyQueries()).Count -le $spotify; $i++) { Start-Sleep -Milliseconds 100 }
for ($i = 0; $i -lt 50 -and (Test-Syncing $h); $i++) { Start-Sleep -Milliseconds 100 }
Start-Sleep -Milliseconds 300
Check 'Synced: no search, the videos kept' "$(@($Web.YtmQueries()).Count - $searches) | $(Get-SavedTrack $LostKey) | $(Get-SavedTrack $MergedKey)" (
	"0 | pending [$LostKey] customVid01  | pending [$MergedKey] splitVid001 ")
Stop-App $app
