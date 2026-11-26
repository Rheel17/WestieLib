# The Choose a match dialog: opening it, the candidates it lists (the current match, the library's YouTube Music
# entries and the search results, in the Matcher's ranking), searching again, pasted links, and what Use this match
# does: confirm the current match, merge into the library's track, set an override, or split an entry out of a merged
# track.
# Dot-sourced by ui-checks.ps1.

$PicksId = '8pickspickspickspicksp'
$TellKey = 'spotify:spTellMe00000000000001'
$LostKey = 'spotify:spNoMatch0000000000002'
$MergedKey = 'spotify:spMerged00000000000003'

# A track as "status [sources] override failure", or "-" when there is none
function Get-SavedTrack([string]$key) {
	$track = (Get-SavedState).tracks.$key
	if (-not $track) { return '-' }
	"$($track.status) [$($track.sources -join ',')] $($track.ytmOverride) $($track.failure)"
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

function Get-CandidateList($dialog) { [W]::GetDlgItem($dialog, 1204) }

# The list's rows without the play column, which has no text, then what it reads while it has none (its window text)
function Get-CandidateRows($dialog) {
	$list = Get-CandidateList $dialog
	$count = [int][W]::SendMessageW($list, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero)
	$rows = @(if ($count -gt 0) { 0..($count - 1) | ForEach-Object { [W]::ListViewRow($list, $_) -replace ' \| $', '' } }) -join ' / '
	"[$rows] $([W]::TextOf($list))"
}

# The window's size in DIPs
function Get-DialogDips($dialog) {
	$r = New-Object W+RECT
	[void][W]::GetWindowRect($dialog, [ref]$r)
	"$([math]::Round(($r.Right - $r.Left) * 96 / $script:dpi)) x $([math]::Round(($r.Bottom - $r.Top) * 96 / $script:dpi))"
}

function Set-DialogDips($dialog, [int]$width, [int]$height) {
	# SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE
	[void][W]::SetWindowPos($dialog, [IntPtr]::Zero, 0, 0, (S $width), (S $height), 0x16)
}

# The controls the dialog moves, by id, with their rects in its client area, and the Title column's width
$AnchoredIds = [ordered]@{ heading = 1200; spotify = 1207; list = 1204; link = 1205; play = 1208; ok = 1; grip = 1209 }
function Get-DialogLayout($dialog) {
	$layout = @{}
	foreach ($name in $AnchoredIds.Keys) { $layout[$name] = [W]::RectIn([W]::GetDlgItem($dialog, $AnchoredIds[$name]), $dialog) }
	$layout['title'] = [int](([W]::ListViewColumns((Get-CandidateList $dialog)) -split ' \| ')[0] -replace '^Title ', '')
	$layout
}

# How far each control's left, top, right and bottom edges moved, and the Title column's width grew
function Get-LayoutMoves($from, $to) {
	$moves = foreach ($name in $AnchoredIds.Keys) {
		$a = $from[$name]
		$b = $to[$name]
		"$name $($b.Left - $a.Left) $($b.Top - $a.Top) $($b.Right - $a.Right) $($b.Bottom - $a.Bottom)"
	}
	(@($moves) + "title $($to['title'] - $from['title'])") -join ' | '
}

# Waits for the searches the dialog started after $searched searches had reached FakeWeb, then for their answer
function Wait-CandidateRows($dialog, [int]$searched, [int]$count = 1) {
	for ($i = 0; $i -lt 50 -and @($Web.YtmQueries()).Count -lt $searched + $count; $i++) { Start-Sleep -Milliseconds 100 }
	for ($i = 0; $i -lt 50 -and [W]::TextOf((Get-CandidateList $dialog)) -like 'Searching*'; $i++) { Start-Sleep -Milliseconds 100 }
	Get-CandidateRows $dialog
}

# The link's detection line, then whether the link's play button and Use this match are enabled
function Get-ChoiceState($dialog) {
	"$([W]::TextOf([W]::GetDlgItem($dialog, 1206))) | $([W]::IsWindowEnabled([W]::GetDlgItem($dialog, 1208))) $([W]::IsWindowEnabled([W]::GetDlgItem($dialog, 1)))"
}

function Set-YtmLink($dialog, [string]$link) { [W]::SetEditText([W]::GetDlgItem($dialog, 1205), $link) }

# Whether "Open by itself when an update needs a choice" is checked (BM_GETCHECK), and whether Next shows
function Get-OpenByItself($dialog) {
	"$([W]::SendMessageW([W]::GetDlgItem($dialog, 1212), 0xF0, [IntPtr]::Zero, [IntPtr]::Zero) -eq 1) $([W]::IsWindowVisible([W]::GetDlgItem($dialog, 1213)))"
}

# The dialog an update opens, which waits for the fetch and the searches before it
function Wait-ChoiceDialog($hwnd) {
	for ($i = 0; $i -lt 150; $i++) {
		$window = [W]::OwnedWindow($hwnd)
		if ($window -ne [IntPtr]::Zero) { return $window }
		Start-Sleep -Milliseconds 100
	}
	[IntPtr]::Zero
}

function Get-SavedOpenByItself { (Get-Content $SettingsFile -Raw -Encoding UTF8 | ConvertFrom-Json).openChooseMatch }

function Use-ThisResult($dialog) {
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
}

function Open-ChooseMatch($hwnd, [scriptblock]$open) {
	$searched = @($Web.YtmQueries()).Count
	& $open
	$dialog = Wait-OwnedWindow $hwnd
	[pscustomobject]@{ Dialog = $dialog; Before = $searched }
}

# A YouTube Music playlist with Tell Me, another song and the source of a merged track, and a Spotify playlist: Tell Me
# matched to a live version (78%, to review, as an earlier Matcher left it), a song without a match, and one merged into
# the YouTube Music track. That one's match has the key an earlier version wrote, videoId.
$playlists = '"ytm:PLlib1": {"url": "https://music.youtube.com/playlist?list=PLlib1", "name": "Library list", "addedAt": "2026-01-01T12:00:00Z", ' +
	'"sources": ["ytm:libSong0001", "ytm:otherSong01", "ytm:sharedVid01"]}, ' +
	"`"spotify:$PicksId`": {`"url`": `"https://open.spotify.com/playlist/$PicksId`", `"name`": `"Picks`", `"addedAt`": `"2026-01-02T12:00:00Z`", " +
	"`"sources`": [`"$TellKey`", `"$LostKey`", `"$MergedKey`"]}"
$sources = @(
	'"ytm:libSong0001": {"title": "Tell Me", "artists": ["Rae"], "durationMs": 201000, "track": "ytm:libSong0001"}',
	'"ytm:otherSong01": {"title": "Something Else", "artists": ["Nobody"], "durationMs": 100000, "track": "ytm:otherSong01"}',
	'"ytm:sharedVid01": {"title": "Shared Song", "artists": ["Both"], "durationMs": 200000, "track": "ytm:sharedVid01"}',
	("`"$TellKey`": {`"title`": `"Tell Me`", `"artists`": [`"Rae`"], `"album`": `"Tell`", `"durationMs`": 200000, " +
		'"match": {"ytmId": "reviewVid01", "confidence": 0.78, "title": "Tell Me (Live)", "artists": ["Rae"], "durationMs": 260000}, ' +
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
	[FakeWeb]::Song('karaoke0001', 'Tell Me (Karaoke)', @('Sing'), 'Karaoke', '3:30'),
	[FakeWeb]::Song('longTell001', 'Tell Me', @('Rae'), 'Tell', '3:45')
)
$Web.YtmResults['Tell Me Rae'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('otherTell01', 'Tell Me', @('Rae'), 'Other', '3:22'))
$Web.YtmResults['Gone Lost Song'] = New-Object Collections.Generic.List[string]
[void]$Web.YtmBrokenQueries.Add('Broken query')

"--- Run 1: choosing matches"
# Its updates leave tracks to review, which would open the dialog by itself until Run 3
Set-SavedSetting openChooseMatch $false
[IO.File]::WriteAllText($StateFile, "{`"version`": 1, `"playlists`": {$playlists}, `"sources`": {$($sources -join ', ')}, `"tracks`": {$($tracks -join ', ')}}")
$app = Start-App
$h = $app.Hwnd
$script:TallRows = 1, 2
Click-PlaylistRow $h 2
$table = Get-TrackTable $h

Select-ListRow $table 0
Check 'Tell Me, to review: the card offers Choose match' ((Wait-TrackCard $h $TellKey) -like "* / Needs review / * / $ChooseMatch / Source / *") 'True'
Update-MenuStates $h
$grayed = [W]::ItemsWithState([W]::GetMenu($h), 0x1)
Check '  menu: Confirm match and Choose match enabled' "$($grayed -like '*&Confirm match*') $($grayed -like '*C&hoose match*')" 'False False'
$open = Open-ChooseMatch $h { Click-CardButton $h $ChooseMatch }
$dialog = $open.Dialog
Check '  the card''s button opens it' "$([W]::ClassOf($dialog)) $([W]::TextOf($dialog))" '#32770 Choose a match'
Check '  opened by hand: Open by itself off, as settings.json says | Next hidden' (Get-OpenByItself $dialog) 'False False'
Check '  the track | the current match | the query' (
	"$([W]::TextOf([W]::GetDlgItem($dialog, 1200))) | $([W]::TextOf([W]::GetDlgItem($dialog, 1201))) | $([W]::EditText([W]::GetDlgItem($dialog, 1202)))") (
	"Tell Me by Rae (Tell, 3:20) | Current match: Tell Me (Live), 78% confidence | Rae Tell Me")
# The result on the track's album ranks above the library's song, a cut 25 s longer below them, and the live version
# above the karaoke by someone else
Check '  the Matcher''s query searched: the results, the library''s song and the current match, ranked' (Wait-CandidateRows $dialog $open.Before) (
	'[Tell Me | Rae | Tell | 3:20 | Same recording / Tell Me (in library) | Rae |  | 3:21 | Same recording / Tell Me | Rae | Tell | 3:45 | Check length / ' +
	'Tell Me (Live) (current) | Rae |  | 4:20 | Other recording / Tell Me (Karaoke) | Sing | Karaoke | 3:30 | Other artist] ' +
	'No results. Try fewer words, or paste a link below.')
Check '  columns after Title, the last for the play buttons' ([W]::ListViewColumns((Get-CandidateList $dialog)) -replace '^Title \d+ \| ', '') (
	"Artists $(S 115) | Album $(S 115) | Length $(S 52) | Match $(S 110) |  $(S 26)")
Check '  opens at 900 x 600, the search field focused' "$(Get-DialogDips $dialog) $([W]::FocusOf($dialog) -eq [W]::GetDlgItem($dialog, 1202))" '900 x 600 True'
$playButtons = foreach ($id in 1207, 1208) {
	$button = [W]::GetDlgItem($dialog, $id)
	# BS_ICON
	"$([W]::TextOf($button)) $([W]::IsWindowEnabled($button)) $(([W]::GetWindowLongW($button, -16) -band 0x40) -ne 0)"
}
Check '  play buttons: Spotify enabled, the link''s waiting for a link' ($playButtons -join ' | ') 'Play on Spotify True True | Play on YouTube Music False True'
if ($Screenshot) { Save-Screenshot $dialog ("$($Screenshot -replace '\.png$', '')-choose-match.png") }
$defaultLayout = Get-DialogLayout $dialog
Set-DialogDips $dialog 1000 700
Check 'Resized to 1000 x 700: what the anchors move' (Get-LayoutMoves $defaultLayout (Get-DialogLayout $dialog)) (
	"heading 0 0 $(S 100) 0 | spotify $(S 100) 0 $(S 100) 0 | list 0 0 $(S 100) $(S 100) | link 0 $(S 100) $(S 100) $(S 100) | " +
	"play $(S 100) $(S 100) $(S 100) $(S 100) | ok $(S 100) $(S 100) $(S 100) $(S 100) | grip $(S 100) $(S 100) $(S 100) $(S 100) | title $(S 100)")
$client = New-Object W+RECT
[void][W]::GetClientRect($dialog, [ref]$client)
$grip = [W]::RectIn([W]::GetDlgItem($dialog, 1209), $dialog)
# The template's dialog units round it a pixel past the edge at some scales
Check '  the size grip reaches the corner' "$($grip.Right -ge $client.Right) $($grip.Bottom -ge $client.Bottom)" 'True True'
Set-DialogDips $dialog 300 200
$smallest = Get-DialogDips $dialog
$width, $height = [int[]]($smallest -split ' x ')
Check-That '  shrunk to 300 x 200: held at the template''s size' $smallest ($width -ge 640 -and $width -le 680 -and $height -ge 480 -and $height -le 540) '640-680 x 480-540'
$smallLayout = Get-DialogLayout $dialog
Check '  there, the controls stand where the template has them' ((Get-LayoutMoves $smallLayout $defaultLayout) -replace "\b$(S (900 - $width))\b", 'W' -replace "\b$(S (600 - $height))\b", 'H') (
	'heading 0 0 W 0 | spotify W 0 W 0 | list 0 0 W H | link 0 H W H | play W H W H | ok W H W H | grip W H W H | title W')
Set-DialogDips $dialog 1000 700
Check '  nothing chosen yet: no line, buttons disabled' (Get-ChoiceState $dialog) ' | False False'
Select-ListRow (Get-CandidateList $dialog) 1
Check '  a row selected: Use this match enabled' (Get-ChoiceState $dialog) ' | False True'
$links = @(
	@{ Link = 'hello'; Line = 'Not a YouTube link | False True' },
	@{ Link = ' https://youtu.be/libSong0001?si=x '; Line = "YouTube Music id libSong0001 Tell Me | True True" },
	@{ Link = 'MUSIC.youtube.com/watch?list=x&v=abcdefghijk'; Line = "YouTube Music id abcdefghijk | True True" },
	@{ Link = 'https://m.youtube.com/shorts/abcdefghijk'; Line = "YouTube Music id abcdefghijk | True True" },
	@{ Link = 'abcdefghij'; Line = 'Not a YouTube link | False True' },
	@{ Link = ''; Line = ' | False True' }
)
foreach ($case in $links) {
	Set-YtmLink $dialog $case.Link
	Check "  link '$($case.Link)'" (Get-ChoiceState $dialog) $case.Line
}

$searched = @($Web.YtmQueries()).Count
[W]::SetEditText([W]::GetDlgItem($dialog, 1202), 'Tell Me Rae')
[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1203), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Check 'Search: its results, the current match and the library''s song stay' (Wait-CandidateRows $dialog $searched) (
	'[Tell Me (in library) | Rae |  | 3:21 | Same recording / Tell Me | Rae | Other | 3:22 | Same recording / ' +
	'Tell Me (Live) (current) | Rae |  | 4:20 | Other recording] No results. Try fewer words, or paste a link below.')
$searched = @($Web.YtmQueries()).Count
[W]::SetEditText([W]::GetDlgItem($dialog, 1202), 'Broken query')
[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1203), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Check '  a search that fails' (Wait-CandidateRows $dialog $searched) (
	'[Tell Me (in library) | Rae |  | 3:21 | Same recording / Tell Me (Live) (current) | Rae |  | 4:20 | Other recording] ' +
	'Couldn''t search YouTube Music: HTTP 500 from 127.0.0.1')
Select-ListRow (Get-CandidateList $dialog) 1
Use-ThisResult $dialog
for ($i = 0; $i -lt 50 -and -not (Get-SavedState).sources.$TellKey.match.confirmed; $i++) { Start-Sleep -Milliseconds 100 }
Check 'Use this match on the current match: confirmed, nothing else' "$(Get-SavedTrack $TellKey) | $((Get-SavedState).sources.$TellKey.match.confirmed)" (
	"pending [$TellKey]   | True")
Check '  Waiting now' ((Wait-TrackCard $h $TellKey) -like '* / Status / Waiting / *') 'True'
$sharedMatch = (Get-SavedState).sources.$MergedKey.match
Check '  saved: the earlier version''s videoId as ytmId' "$($sharedMatch.ytmId) $($null -eq $sharedMatch.PSObject.Properties['videoId'])" 'sharedVid01 True'

$open = Open-ChooseMatch $h { Send-Command $h 'C&hoose match...' }
$dialog = $open.Dialog
Check 'Opened again: the size it was closed at' (Get-DialogDips $dialog) '1000 x 700'
[void](Wait-CandidateRows $dialog $open.Before)
# Under the result on the track's album
Select-ListRow (Get-CandidateList $dialog) 1
Use-ThisResult $dialog
Check 'Track > Choose match, the library''s song: merged into its track' "$(Wait-SavedTrack 'ytm:libSong0001' '*,spotify*') | $(Get-SavedTrack $TellKey)" (
	"pending [ytm:libSong0001,$TellKey]   | -")

Select-ListRow $table 1
Check 'Lost Song, no match: the card offers Choose match and Retry' ((Wait-TrackCard $h $LostKey) -like "* / No match found. / * / $ChooseMatch / Retry / Source / *") 'True'
# A row that needs a match opens the dialog on a double-click
$open = Open-ChooseMatch $h { Double-Click-Row $table 1 }
$dialog = $open.Dialog
Check '  double-clicked: the dialog' "$([W]::ClassOf($dialog)) $([W]::TextOf($dialog))" '#32770 Choose a match'
Check '  no current match, no results' "$([W]::TextOf([W]::GetDlgItem($dialog, 1201))) | $(Wait-CandidateRows $dialog $open.Before)" (
	'No match found. | [] No results. Try fewer words, or paste a link below.')
Set-YtmLink $dialog 'https://www.youtube.com/watch?v=customVid01'
Use-ThisResult $dialog
Check '  a pasted link: the override, no failure' (Wait-SavedTrack $LostKey '*customVid01*') "pending [$LostKey] customVid01 "
Check '  its card' ((Wait-TrackCard $h $LostKey) -like "* / Status / Waiting / * / ID / customVid01 / Chosen by / Override / File / *") 'True'
Double-Click-Row $table 1
Start-Sleep -Milliseconds 500
Check '  matched now: a double-click opens nothing' ([W]::OwnedWindow($h) -eq [IntPtr]::Zero) 'True'

Select-ListRow $table 2
[void](Wait-TrackCard $h $MergedKey)
$open = Open-ChooseMatch $h { Send-Command $h 'C&hoose match...' }
$dialog = $open.Dialog
Check 'Shared Song, merged: the current match is the playlist''s' "$([W]::TextOf([W]::GetDlgItem($dialog, 1201))) | $(Wait-CandidateRows $dialog $open.Before)" (
	'Current match: Shared Song, from a YouTube Music playlist | [Shared Song (current) | Both |  | 3:20 | Same recording] ' +
	'No results. Try fewer words, or paste a link below.')
Set-YtmLink $dialog 'splitVid001'
Use-ThisResult $dialog
Check '  another YouTube Music id: the entry splits out with it' "$(Wait-SavedTrack $MergedKey '*splitVid001*') | $(Get-SavedTrack 'ytm:sharedVid01')" (
	"pending [$MergedKey] splitVid001  | pending [ytm:sharedVid01]  ")

# The tracks with a YouTube Music id of the user's aren't matched again
$Web.SpotifyNames[$PicksId] = 'Picks'
$Web.SpotifyItems[$PicksId] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Track('spTellMe00000000000001', 'Tell Me', @('Rae'), 'Tell', 200000),
	[FakeWeb]::Track('spNoMatch0000000000002', 'Lost Song', @('Gone'), 'Away', 180000),
	[FakeWeb]::Track('spMerged00000000000003', 'Shared Song', @('Both'), 'Shared', 200000)
)
$searches = @($Web.YtmQueries()).Count
$spotify = @($Web.SpotifyQueries()).Count
Send-Command $h 'Update &selected playlist'
for ($i = 0; $i -lt 50 -and @($Web.SpotifyQueries()).Count -le $spotify; $i++) { Start-Sleep -Milliseconds 100 }
for ($i = 0; $i -lt 50 -and (Test-Syncing $h); $i++) { Start-Sleep -Milliseconds 100 }
Start-Sleep -Milliseconds 300
Check 'Synced: no search, the overrides kept' "$(@($Web.YtmQueries()).Count - $searches) | $(Get-SavedTrack $LostKey) | $(Get-SavedTrack $MergedKey)" (
	"0 | pending [$LostKey] customVid01  | pending [$MergedKey] splitVid001 ")
Stop-App $app
$saved = Get-ItemProperty $KeyPath
Check '  saved: the dialog''s size' "$($saved.ChooseMatchWidthDips) x $($saved.ChooseMatchHeightDips)" '1000 x 700'

"--- Run 2: the saved size, and an override with the key an earlier version wrote"
Set-SavedWindow @{ ChooseMatchWidthDips = 820; ChooseMatchHeightDips = 560 }
[IO.File]::WriteAllText($StateFile, ([IO.File]::ReadAllText($StateFile) -replace '"ytmOverride": "customVid01"', '"videoOverride": "customVid01"'))
$app = Start-App
$h = $app.Hwnd
$script:TallRows = 1, 2
Click-PlaylistRow $h 2
Select-ListRow (Get-TrackTable $h) 1
Check 'The earlier version''s videoOverride reads as the override' ((Wait-TrackCard $h $LostKey) -like '* / ID / customVid01 / Chosen by / Override / *') 'True'
$open = Open-ChooseMatch $h { Send-Command $h 'C&hoose match...' }
Check 'Choose a match opens at the saved size' (Get-DialogDips $open.Dialog) '820 x 560'
[void](Wait-CandidateRows $open.Dialog $open.Before)
[void][W]::PostMessageW([W]::GetDlgItem($open.Dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Wait-Closed $open.Dialog
Stop-App $app

"--- Run 3: opening by itself during an update"
# Drift Away's only result is 25 s longer, so it is to review; Nowhere Song finds nothing
Set-SavedSetting openChooseMatch $true
$AutoId = '9autoautoautoautoautoa'
$Web.SpotifyNames[$AutoId] = 'Auto picks'
$Web.SpotifyItems[$AutoId] = [Collections.Generic.List[string]]@(
	[FakeWeb]::Track('spDriftAway00000000004', 'Drift Away', @('Kai'), 'Drift', 200000),
	[FakeWeb]::Track('spNowhere0000000000005', 'Nowhere Song', @('Ghost'), 'Nowhere', 180000)
)
$Web.YtmResults['Kai Drift Away'] = [Collections.Generic.List[string]]@([FakeWeb]::Song('driftLong01', 'Drift Away', @('Kai'), 'Drift', '3:45'))
$Web.YtmResults['Ghost Nowhere Song'] = New-Object Collections.Generic.List[string]
[IO.File]::WriteAllText($StateFile, "{`"version`": 1, `"playlists`": {`"spotify:$AutoId`": {`"url`": `"https://open.spotify.com/playlist/$AutoId`", " +
	"`"name`": `"Auto picks`", `"addedAt`": `"2026-01-03T12:00:00Z`"}}}")
$app = Start-App
$h = $app.Hwnd
Send-Command $h 'Update &all'
$first = Wait-ChoiceDialog $h
Check 'Update: the dialog opens by itself for Drift Away, to review' "$([W]::TextOf($first)) | $([W]::TextOf([W]::GetDlgItem($first, 1200)))" (
	'Choose a match | Drift Away by Kai (Drift, 3:20)')
# Nowhere Song may be searched still as it opens
for ($i = 0; $i -lt 50 -and -not [W]::IsWindowVisible([W]::GetDlgItem($first, 1213)); $i++) { Start-Sleep -Milliseconds 100 }
Check '  Open by itself on | Next, once Nowhere Song waits too' (Get-OpenByItself $first) 'True True'
[void][W]::PostMessageW([W]::GetDlgItem($first, 1213), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Wait-Closed $first
$second = Wait-ChoiceDialog $h
Check '  Next: the dialog for Nowhere Song, no match, no Next' (
	"$([W]::TextOf([W]::GetDlgItem($second, 1200))) | $([W]::TextOf([W]::GetDlgItem($second, 1201))) | $(Get-OpenByItself $second)") (
	'Nowhere Song by Ghost (Nowhere, 3:00) | No match found. | True False')
# BM_CLICK, as a click would
[void][W]::SendMessageW([W]::GetDlgItem($second, 1212), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Check '  Open by itself turned off' (Get-OpenByItself $second) 'False False'
[void][W]::PostMessageW([W]::GetDlgItem($second, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Wait-Closed $second
Start-Sleep -Milliseconds 500
Check '  Cancel: no other dialog | saved off | the tracks as the update left them' (
	"$([W]::OwnedWindow($h) -eq [IntPtr]::Zero) | $(Get-SavedOpenByItself) | $(Get-SavedTrack 'spotify:spDriftAway00000000004') | $(Get-SavedTrack 'spotify:spNowhere0000000000005')") (
	'True | False | pending [spotify:spDriftAway00000000004]   | failed [spotify:spNowhere0000000000005]  noMatch')
Stop-App $app
