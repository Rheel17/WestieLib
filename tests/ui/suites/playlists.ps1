# Playlists: adding them, keeping them in state.json across restarts, the rows, selecting with the mouse and the keys,
# the inspector's cards, the playlist menu, removing, scrolling a long list, and a state.json that doesn't read.
# Dot-sourced by ui-checks.ps1.

# The playlists in state.json, a file named here, as "key url [name] whether addedAt is an ISO 8601 time"
function Get-SavedPlaylists([string]$file = $StateFile) {
	if (-not (Test-Path $file)) { return 'no file' }
	$json = Get-Content $file -Raw -Encoding UTF8 | ConvertFrom-Json
	@($json.playlists.PSObject.Properties | ForEach-Object {
		"$($_.Name) $($_.Value.url) [$($_.Value.name)] $($_.Value.addedAt -match '^\d{4}-\d\d-\d\dT\d\d:\d\d:\d\dZ$')"
	}) -join ' / '
}

# Welcome screen and tracks panel visible; the left panel's "No playlists yet." visible; whether the inspector shows a card
function Get-PlaylistScreens($hwnd) {
	$empty = ([W]::Children((Get-RowPanels $hwnd)[0], 'WestieLib.Label'))[0]
	"welcome $([W]::IsWindowVisible((Get-Panel $hwnd 'Welcome'))), tracks $([W]::IsWindowVisible((Get-Panel $hwnd 'Tracks'))), " +
		"no playlists $([W]::IsWindowVisible($empty)), card $($null -ne (Get-InspectorCard $hwnd))"
}

$WithPlaylists = 'welcome False, tracks True, no playlists False, card True'

# The list starts under the rule, as wide as the panel, and its button follows the rows, lined up with the names
function Check-PlaylistRows([string]$what, $hwnd, [int]$count) {
	$panel = (Get-RowPanels $hwnd)[0]
	$list = Get-PlaylistList $hwnd
	$listTop = (S 4) + (2 + (S 5) + (S 18) + (S 16) + (S 6)) + (S 5) + 1 + (S 5)
	$r = [W]::RectIn($list, $panel)
	$button = [W]::RectIn(([W]::Children($list, 'Button'))[0], $list)
	$text = (S 3) + 1 + (S 7) + (S 16) + (S 6)
	$actual = "$($r.Left),$($r.Top) $($r.Right - $r.Left) | $($button.Left),$($button.Top)"
	Check "$what list | button" $actual "0,$listTop $(Get-ClientWidth $panel) | $text,$(Get-ListButtonTop $count)"
}

# Whether the commands that need a selected playlist are enabled
function Get-PlaylistCommands($hwnd) {
	Update-MenuStates $hwnd
	$menu = [W]::GetMenu($hwnd)
	$states = '&Remove playlist...', 'Sync &selected playlist' | ForEach-Object { ([W]::GetMenuState($menu, [W]::CommandOf($menu, $_), 0) -band 0x1) -eq 0 }
	"Remove $($states[0]), Sync selected $($states[1])"
}

function Press-ListKey($hwnd, [int]$key) { [void][W]::SendMessageW((Get-PlaylistList $hwnd), 0x100, [IntPtr]$key, [IntPtr]1) }

# All tracks is selected at the start. A click on a row selects it, gives the list the focus and enables the playlist
# commands; a click between the rows changes nothing. The keys move the selection, with All tracks before the first
# row. Ends with row 1 selected, for the screenshot.
function Check-Selection($hwnd) {
	$list = Get-PlaylistList $hwnd
	Check 'Selection at the start | commands' "$(Get-Selection $hwnd 2) | $(Get-PlaylistCommands $hwnd)" 'All tracks | Remove False, Sync selected False'
	Click-PlaylistRow $hwnd 2
	Check '  click row 2' "$(Get-Selection $hwnd 2) | $(Get-PlaylistCommands $hwnd)" 'row 2 | Remove True, Sync selected True'
	Check '  the list has the focus' ([W]::FocusOf($hwnd) -eq $list) 'True'
	Send-Click $list (S 20) ((Get-ListButtonTop 2) - (S 4))
	Check '  a click under the rows' (Get-Selection $hwnd 2) 'row 2'
	Click-AllTracks $hwnd
	Check '  click All tracks' "$(Get-Selection $hwnd 2) | $(Get-PlaylistCommands $hwnd)" 'All tracks | Remove False, Sync selected False'
	Check '  the list keeps the focus' ([W]::FocusOf($hwnd) -eq $list) 'True'
	$keys = @(
		@{ Name = 'Down'; Key = 0x28; Selection = 'row 1' },
		@{ Name = 'Up'; Key = 0x26; Selection = 'All tracks' },
		@{ Name = 'Up again'; Key = 0x26; Selection = 'All tracks' },
		@{ Name = 'End'; Key = 0x23; Selection = 'row 2' },
		@{ Name = 'Down again'; Key = 0x28; Selection = 'row 2' },
		@{ Name = 'Home'; Key = 0x24; Selection = 'All tracks' },
		@{ Name = 'Page Down'; Key = 0x22; Selection = 'row 2' },
		@{ Name = 'Page Up'; Key = 0x21; Selection = 'All tracks' },
		@{ Name = 'Down'; Key = 0x28; Selection = 'row 1' }
	)
	foreach ($key in $keys) {
		Press-ListKey $hwnd $key.Key
		Check "  $($key.Name)" (Get-Selection $hwnd 2) $key.Selection
	}
}

# How All tracks and each of the $count rows look, from the same pixels as Get-Selection: blue (selected or pressed),
# hot, or - (neither)
function Get-RowLooks($hwnd, [int]$count) {
	$names = @{ 'cce8ff' = 'blue'; 'e5f3ff' = 'hot'; 'ffffff' = '-' }
	$points = @(1..$count | ForEach-Object { (S 3) + 3; (Get-PlaylistRowTop $hwnd $_) + 3 })
	$colors = @([W]::PixelsIn($hwnd, (Get-AllTracksRow $hwnd), @((S 4), (S 4)))) + @([W]::PixelsIn($hwnd, (Get-PlaylistList $hwnd), [int[]]$points))
	($colors | ForEach-Object { if ($names.ContainsKey($_)) { $names[$_] } else { $_ } }) -join ' | '
}

function Send-Mouse($window, [int]$message, [int]$buttons, [int]$x, [int]$y) {
	[void][W]::SendMessageW($window, $message, [IntPtr]$buttons, (LParam $x $y))
}

# Rows hover and press as in Explorer's navigation pane, with two rows and row 1 selected: a press shows on the pressed
# row only while the mouse is over it, and selects only when the button comes up there; a lost capture cancels the
# press. Ends with row 1 selected and nothing hot.
function Check-RowMouse($hwnd) {
	$list = Get-PlaylistList $hwnd
	$allTracks = Get-AllTracksRow $hwnd
	$x = S 20
	$row = { param([int]$index) (Get-PlaylistRowTop $hwnd $index) + [int]((Get-PlaylistRowHeight) / 2) }
	$below = (Get-ListButtonTop 2) - (S 4)
	# A hover only lasts while the real cursor is there, which a check can't move without touching the user's mouse. So
	# these only show that every hover asks for WM_MOUSELEAVE, which Windows sends at once while the cursor is elsewhere,
	# also after a click.
	$steps = @(
		@{ Name = 'Clicked earlier, cursor elsewhere: no hover'; Do = { }; Looks = '- | blue | -' },
		@{ Name = 'Hover row 2, cursor elsewhere: gone'; Do = { Send-Mouse $list 0x200 0 $x (& $row 2); Start-Sleep -Milliseconds 100 }; Looks = '- | blue | -' },
		@{ Name = 'Hover All tracks, cursor elsewhere: gone'; Do = { Send-Mouse $allTracks 0x200 0 $x (S 10); Start-Sleep -Milliseconds 100 }; Looks = '- | blue | -' },
		@{ Name = 'Press row 2'; Do = { Send-Mouse $list 0x201 1 $x (& $row 2) }; Looks = '- | blue | blue' },
		@{ Name = '  move onto row 1'; Do = { Send-Mouse $list 0x200 1 $x (& $row 1) }; Looks = '- | blue | -' },
		@{ Name = '  back onto row 2'; Do = { Send-Mouse $list 0x200 1 $x (& $row 2) }; Looks = '- | blue | blue' },
		@{ Name = '  under the rows'; Do = { Send-Mouse $list 0x200 1 $x $below }; Looks = '- | blue | -' },
		@{ Name = '  release there: no selection'; Do = { Send-Mouse $list 0x202 0 $x $below }; Looks = '- | blue | -' },
		@{ Name = 'Press All tracks, release above it'; Do = { Send-Mouse $allTracks 0x201 1 $x (S 10); Send-Mouse $allTracks 0x202 0 $x -5 }; Looks = '- | blue | -' },
		@{ Name = 'Press row 2, lose the capture, release'; Do = { Send-Mouse $list 0x201 1 $x (& $row 2); Send-Mouse $list 0x1F 0 0 0; Send-Mouse $list 0x202 0 $x (& $row 2) }; Looks = '- | blue | -' },
		@{ Name = 'Press and release row 2'; Do = { Send-Mouse $list 0x201 1 $x (& $row 2); Send-Mouse $list 0x202 0 $x (& $row 2) }; Looks = '- | - | blue' },
		@{ Name = 'Press and release row 1'; Do = { Send-Mouse $list 0x201 1 $x (& $row 1); Send-Mouse $list 0x202 0 $x (& $row 1) }; Looks = '- | blue | -' }
	)
	foreach ($step in $steps) {
		& $step.Do
		Check $step.Name (Get-RowLooks $hwnd 2) $step.Looks
	}
}

# Keys bring the selected row into view: End scrolls the last row's gap to the list's bottom, and Down from All tracks
# brings the first row back to the top. Page Down moves by the rows that fit.
function Check-SelectionScrolling($hwnd, [int]$count) {
	$list = Get-PlaylistList $hwnd
	Press-ListKey $hwnd 0x23
	$bottom = (Get-PlaylistRowTop $hwnd $count) + (Get-PlaylistRowHeight) + (S 1)
	Check 'Many playlists: End | last row bottom' "$(Get-Selection $hwnd $count) | $bottom" "row $count | $(Get-ClientHeight $list)"
	Press-ListKey $hwnd 0x24
	Check '  Home leaves the list where it is' "$(Get-Selection $hwnd $count) | $([W]::GetScrollPos($list, 1) -gt 0)" 'All tracks | True'
	Press-ListKey $hwnd 0x28
	Check '  Down' "$(Get-Selection $hwnd $count) | $([W]::GetScrollPos($list, 1))" 'row 1 | 0'
	$page = [math]::Floor((Get-ClientHeight $list) / ((Get-PlaylistRowHeight) + (S 1)))
	Press-ListKey $hwnd 0x22
	Check '  Page Down' (Get-Selection $hwnd $count) "row $(1 + $page)"
	Press-ListKey $hwnd 0x24
	Press-ListKey $hwnd 0x28
	Press-ListKey $hwnd 0x26
	Check '  Home, Down, Up' "$(Get-Selection $hwnd $count) | $([W]::GetScrollPos($list, 1))" 'All tracks | 0'
}

# More playlists than fit: the list gets a scroll bar. Scrolled to the bottom, the button ends the list with its
# padding under it; the wheel and the top of the scroll bar bring the rows back.
function Check-PlaylistScrolling($hwnd, [int]$count) {
	$list = Get-PlaylistList $hwnd
	$client = New-Object W+RECT
	$frame = New-Object W+RECT
	[void][W]::GetClientRect($list, [ref]$client)
	[void][W]::GetWindowRect($list, [ref]$frame)
	$bar = ($frame.Right - $frame.Left) - $client.Right
	Check-That 'Many playlists: a scroll bar' $bar ($bar -gt 0) 'wider than 0'
	$button = ([W]::Children($list, 'Button'))[0]
	[void][W]::SendMessageW($list, 0x115, [IntPtr]7, [IntPtr]::Zero)
	Check '  SB_BOTTOM: the button, then the padding' ($client.Bottom - [W]::RectIn($button, $list).Bottom) (S 11)
	$top = [W]::RectIn($button, $list).Top
	[void][W]::SendMessageW($list, 0x20A, [IntPtr](120 * 65536), [IntPtr]::Zero)
	$moved = [W]::RectIn($button, $list).Top - $top
	Check-That '  a wheel notch up moves the rows down' $moved ($moved -gt 0) 'more than 0'
	[void][W]::SendMessageW($list, 0x115, [IntPtr]6, [IntPtr]::Zero)
	Check '  SB_TOP: back where it started' ([W]::RectIn($button, $list).Top) (Get-ListButtonTop $count)
}

# Adding playlists stores them in state.json, under their key with the canonical link and the name the look-up found.
# The welcome screen gives way to the tracks panel, and Sync all enables. A link to a playlist already in the list shows
# its name, and Add reads Already added and stays disabled.
function Check-AddingPlaylists($hwnd) {
	$toolbar = ([W]::Children((Get-ToolbarStrip $hwnd), 'ToolbarWindow32'))[0]
	$spotifyKey = "spotify:$SpotifyId $SpotifyLink [$SpotifyName]"
	Check 'Add a Spotify playlist: the line | Add' (Add-Playlist $hwnd "$SpotifyLink`?si=4f1c2e7a9b3d4e5f") "$SpotifyFound | Add True"
	Check '  state.json' (Get-SavedPlaylists) "$spotifyKey True"
	Check '  no state.json.bak for the first save' (Test-Path "$StateFile.bak") 'False'
	Check '  screens' (Get-PlaylistScreens $hwnd) $WithPlaylists
	Check-PlaylistRows '  one row:' $hwnd 1
	Check '  selected' (Get-Selection $hwnd 1) 'row 1'
	Check '  toolbar buttons' ([W]::ToolbarButtons($toolbar)) 'Add playlist, Sync all v, Cancel (disabled), -, Settings'
	$menu = [W]::GetMenu($hwnd)
	Update-MenuStates $hwnd
	Check '  grayed menu items' ([W]::ItemsWithState($menu, 0x1)) ('&Cancel, ' +
		'&Confirm match, C&hoose video..., &Edit track..., &Retry, &Keep, &Move to Recycle Bin..., &Show in folder, Open on &YouTube, Open source &page')

	Send-Command $hwnd '&Add playlist...'
	$dialog = Wait-OwnedWindow $hwnd
	Set-PlaylistLink $dialog "spotify:playlist:$SpotifyId"
	Check 'The same playlist again: line | Add' (Get-LinkLine $dialog) "$SpotifyFound | Already added False"
	$add = [W]::GetDlgItem($dialog, 1)
	$c = New-Object W+RECT
	[void][W]::GetClientRect($add, [ref]$c)
	$width = [W]::TextWidth('Already added', $script:dpi)
	Check-That '  Already added fits its button, with room' "$width of $($c.Right)" ($width + (S 12) -le $c.Right) "at most $($c.Right - (S 12))"
	# IDOK can come without the button, which is disabled
	[void][W]::PostMessageW($dialog, 0x111, [IntPtr]1, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 200
	Check '  IDOK keeps the dialog open' ([W]::IsWindow($dialog)) 'True'
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog

	Check 'Add a YouTube Music playlist' (Add-Playlist $hwnd $YouTubeLink) "$YouTubeFound | Add True"
	$youTubeKey = "ytm:PLx9k2_-Ab $YouTubeLink [$YouTubeName]"
	Check '  state.json' (Get-SavedPlaylists) "$spotifyKey True / $youTubeKey True"
	Check '  state.json.bak keeps the version before' (Get-SavedPlaylists "$StateFile.bak") "$spotifyKey True"
	Check '  the new playlist is selected' (Get-Selection $hwnd 2) 'row 2'
}

# Starts the app with a state.json that isn't JSON: it shows the error, naming the file, instead of its window, exits
# with 1, and leaves the file as it was
function Check-BrokenState {
	$broken = '{"version": 1, "playlists": {'
	[IO.File]::WriteAllText($StateFile, $broken)
	$process = Start-Process -FilePath $Exe -PassThru
	$box = [IntPtr]::Zero
	for ($i = 0; $i -lt 50 -and $box -eq [IntPtr]::Zero; $i++) {
		Start-Sleep -Milliseconds 100
		$box = [W]::WindowOfProcess($process.Id, '#32770')
	}
	$message = @([W]::Children($box, 'Static') | ForEach-Object { [W]::TextOf($_) } | Where-Object { $_ -like '*state.json.bak*' })
	$window = [W]::FindWindowW('WestieLib.MainWindow', [NullString]::Value) -ne [IntPtr]::Zero
	Check 'A broken state.json: error, names the backup, no window' "$([W]::TextOf($box)) $($message.Count) $window" 'WestieLib 1 False'
	[void][W]::PostMessageW($box, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
	if (-not $process.WaitForExit(5000)) { $process.Kill(); throw 'Process did not exit' }
	Check '  exit code, file kept' "$($process.ExitCode) $([IO.File]::ReadAllText($StateFile) -eq $broken)" '1 True'
}

# The card sits inside the inspector's padding. Its title starts at its left edge, the playlist's subtitle where the
# name does, the values 74 + 8 from the left, and the buttons share a row 6 apart.
function Get-CardLayout($hwnd) {
	$inspector = Get-Panel $hwnd 'Inspector'
	$card = Get-InspectorCard $hwnd
	$r = [W]::RectIn($card, $inspector)
	$title = [W]::RectIn(([W]::Children($card, 'WestieLib.CardTitle'))[0], $card)
	$values = @([W]::Children($card, 'WestieLib.Label') | Where-Object { [W]::RectIn($_, $card).Left -gt 0 } | ForEach-Object { [W]::RectIn($_, $card).Left })
	$buttons = @([W]::Children($card, 'Button') | ForEach-Object { [W]::RectIn($_, $card) })
	$gaps = @(for ($i = 1; $i -lt $buttons.Count; $i++) { "$($buttons[$i].Top - $buttons[0].Top),$($buttons[$i].Left - $buttons[$i - 1].Right)" })
	"$($r.Left),$($r.Top) $((Get-ClientWidth $inspector) - $r.Right) | title $($title.Left),$($title.Top) | values $(($values | Sort-Object -Unique) -join ',') | buttons $($gaps -join ' ')"
}

# All tracks shows the Library card with the folder in effect and the totals; a playlist shows its card; the keys move
# between them. Ends on the card of row 1.
function Check-Cards($hwnd) {
	$folder = if (Test-Path $SettingsFile) { (Get-Content $SettingsFile -Raw | ConvertFrom-Json).libraryPath } else { $DefaultLibrary }
	$dot = [char]0xB7
	Click-AllTracks $hwnd
	Check 'All tracks: inspector' (Get-CardTexts $hwnd) "Library card: All tracks / Library / Folder / $folder / Tracks / 0 tracks $dot 0 synced / Open library folder"
	Check '  layout' (Get-CardLayout $hwnd) "$(S 12),$(S 12) $(S 12) | title 0,0 | values $(S 82) | buttons "
	if ($Screenshot) { Save-Screenshot $hwnd ("$($Screenshot -replace '\.png$', '')-library.png") }
	Click-PlaylistRow $hwnd 1
	Check 'Row 1: inspector' (Get-CardTexts $hwnd) ("Playlist card: $SpotifyName / Spotify playlist / Playlist / Link / $SpotifyId / Tracks / 0 / " +
		"Synced / 0/0 / Last sync / Never / Sync playlist / Remove playlist$Ellipsis")
	Check '  layout' (Get-CardLayout $hwnd) "$(S 12),$(S 12) $(S 12) | title 0,0 | values $(S 23),$(S 82) | buttons 0,$(S 6)"
	# The synced value spans the value column, and its bar runs up to 6 before the counts
	$synced = ([W]::Children((Get-InspectorCard $hwnd), 'WestieLib.SyncedValue'))[0]
	$width = Get-ClientWidth $synced
	$end = $width - [W]::TextWidth('0/0', $script:dpi) - (S 6)
	$y = [int]((Get-ClientHeight $synced) / 2)
	$pixels = [W]::PixelsIn($hwnd, $synced, @(0, $y, ($end - 1), $y, $end, $y)) -join ' '
	Check '  synced bar | value right edge' "$pixels | $($width + (S 82))" "e6e6e6 e6e6e6 ffffff | $(Get-ClientWidth (Get-InspectorCard $hwnd))"
	$buttons = @([W]::Children((Get-InspectorCard $hwnd), 'Button') | ForEach-Object { [W]::IsWindowEnabled($_) })
	Check '  buttons enabled' ($buttons -join ' ') 'True True'
	Press-ListKey $hwnd 0x28
	Check 'Down: row 2' (Get-CardTexts $hwnd) ("Playlist card: $YouTubeName / YouTube Music playlist / Playlist / Link / PLx9k2_-Ab / Tracks / 0 / " +
		"Synced / 0/0 / Last sync / Never / Sync playlist / Remove playlist$Ellipsis")
	Press-ListKey $hwnd 0x24
	Check 'Home: All tracks' ([W]::TextOf((Get-InspectorCard $hwnd))) 'Library card'
	Press-ListKey $hwnd 0x28
}

function Wait-PopupMenu($app) {
	for ($i = 0; $i -lt 30; $i++) {
		$popup = [W]::WindowOfProcess($app.Process.Id, '#32768')
		if ($popup -ne [IntPtr]::Zero) { return $popup }
		Start-Sleep -Milliseconds 100
	}
	[IntPtr]::Zero
}

# A popup menu's items, separators as "-", then the grayed ones
function Get-PopupItems($popup) {
	$menu = [W]::SendMessageW($popup, 0x1E1, [IntPtr]::Zero, [IntPtr]::Zero)
	$grayed = @(0..([W]::GetMenuItemCount($menu) - 1) | Where-Object { [W]::GetMenuItemID($menu, $_) -ne 0 -and ([W]::GetMenuState($menu, [uint32]$_, 0x400) -band 0x1) } |
		ForEach-Object { [W]::ItemText($menu, $_) })
	"$([W]::PopupText($menu)) | grayed: $($grayed -join ', ')"
}

# The popup menu's loop takes keys posted to any window of the app's thread
function Close-PopupMenu($app, $popup) {
	[void][W]::PostMessageW($app.Hwnd, 0x100, [IntPtr]0x1B, [IntPtr]1)
	Wait-Closed $popup
}

$PlaylistMenuItems = '&Sync, &Open in browser, &Copy URL, -, &Remove playlist...'

# A right-click on a row selects it first and opens the playlist menu where it was clicked, with every item enabled.
# Shift+F10 sends (-1, -1), and opens it at the selected row's bottom-left corner; with All tracks selected it shows
# nothing. Remove playlist in the menu asks first, and Cancel keeps the playlist. Ends with row 1 selected.
function Check-PlaylistMenu($app) {
	$hwnd = $app.Hwnd
	$list = Get-PlaylistList $hwnd
	$screen = [W]::RectIn($list, [IntPtr]::Zero)
	$x = $screen.Left + (S 20)
	$y = $screen.Top + (Get-PlaylistRowTop $hwnd 2) + (S 10)
	Click-PlaylistRow $hwnd 1
	[void][W]::PostMessageW($list, 0x7B, $list, (LParam $x $y))
	$popup = Wait-PopupMenu $app
	$r = [W]::RectIn($popup, [IntPtr]::Zero)
	Check 'Right-click row 2: menu' "$(Get-PopupItems $popup) | at $($r.Left - $x),$($r.Top - $y)" "$PlaylistMenuItems | grayed:  | at 0,0"
	Close-PopupMenu $app $popup
	Check '  closed, row 2 selected' "$([W]::IsWindowVisible($popup)) | $(Get-Selection $hwnd 2)" 'False | row 2'

	[void][W]::PostMessageW($list, 0x7B, $list, [IntPtr](-1))
	$popup = Wait-PopupMenu $app
	$r = [W]::RectIn($popup, [IntPtr]::Zero)
	$corner = "$($screen.Left + (S 3)),$($screen.Top + (Get-PlaylistRowTop $hwnd 2) + (Get-PlaylistRowHeight))"
	Check 'Shift+F10: at the selected row' "$($r.Left),$($r.Top)" $corner
	# R is Remove playlist's access key
	[void][W]::PostMessageW($hwnd, 0x100, [IntPtr]0x52, [IntPtr]1)
	$dialog = Wait-OwnedWindow $hwnd
	Check '  R: the confirmation' ([W]::TextOf($dialog)) 'Remove playlist'
	[void][W]::PostMessageW($dialog, 0x466, [IntPtr]2, [IntPtr]::Zero)
	Wait-Closed $dialog
	Check '  Cancel keeps it' "$(@((Get-SavedPlaylists) -split ' / ').Count) | $(Get-Selection $hwnd 2)" '2 | row 2'

	Click-AllTracks $hwnd
	[void][W]::PostMessageW($list, 0x7B, $list, [IntPtr](-1))
	Start-Sleep -Milliseconds 500
	Check 'Shift+F10 on All tracks: no menu' ([W]::WindowOfProcess($app.Process.Id, '#32768') -eq [IntPtr]::Zero) 'True'
	Click-PlaylistRow $hwnd 1
}

# Remove playlist asks first; Remove takes the playlist out of state.json and the list, and All tracks is selected.
# Removing the last one, from the playlist card's button, brings back the welcome screen.
function Check-RemovePlaylists($hwnd) {
	Click-PlaylistRow $hwnd 1
	Send-Command $hwnd '&Remove playlist...'
	$dialog = Wait-OwnedWindow $hwnd
	[void][W]::PostMessageW($dialog, 0x466, [IntPtr]1, [IntPtr]::Zero)
	Wait-Closed $dialog
	Check 'Remove row 1: state.json' (Get-SavedPlaylists) "ytm:PLx9k2_-Ab $YouTubeLink [$YouTubeName] True"
	Check '  selection | inspector' "$(Get-Selection $hwnd 1) | $([W]::TextOf((Get-InspectorCard $hwnd)))" 'All tracks | Library card'
	Check-PlaylistRows '  one row left:' $hwnd 1

	Click-PlaylistRow $hwnd 1
	$remove = @([W]::Children((Get-InspectorCard $hwnd), 'Button') | Where-Object { [W]::TextOf($_) -like 'Remove*' })[0]
	[void][W]::PostMessageW($remove, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	$dialog = Wait-OwnedWindow $hwnd
	[void][W]::PostMessageW($dialog, 0x466, [IntPtr]1, [IntPtr]::Zero)
	Wait-Closed $dialog
	Check 'Remove the last, from the card: state.json' "[$(Get-SavedPlaylists)]" '[]'
	Check '  screens' (Get-PlaylistScreens $hwnd) 'welcome True, tracks False, no playlists True, card False'
}

"--- Run 1: adding playlists"
# At a fixed place, rather than wherever Windows cascades it to: Check-RowMouse needs the real cursor off the list
Set-SavedWindow @{}
Install-FakeTools
$app = Start-App
$h = $app.Hwnd
Check 'No playlists yet: screens' (Get-PlaylistScreens $h) 'welcome True, tracks False, no playlists True, card False'
Check-AddingPlaylists $h
Stop-App $app

"--- Run 2: playlists kept"
$app = Start-App
$h = $app.Hwnd
Check 'Screens' (Get-PlaylistScreens $h) $WithPlaylists
Check-PlaylistRows 'Two rows:' $h 2
Check-Selection $h
Check-RowMouse $h
Check-Cards $h
Send-Command $h '&Add playlist...'
$dialog = Wait-OwnedWindow $h
$runs = @(Get-Content $FakeYtDlpRuns -Encoding UTF8).Count
Set-PlaylistLink $dialog $YouTubeLink
Check '  a kept playlist is already added, with its name' (Get-LinkLine $dialog) "$YouTubeFound | Already added False"
Start-Sleep -Milliseconds 500
Check '  which needs no look-up' (@(Get-Content $FakeYtDlpRuns -Encoding UTF8).Count - $runs) 0
[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Wait-Closed $dialog
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-playlists.png") }
Check-PlaylistMenu $app
Check-RemovePlaylists $h
Stop-App $app

"--- Run 3: more playlists than fit"
$many = 20
Write-ManyPlaylists $many
$app = Start-App
$h = $app.Hwnd
Check-PlaylistRows 'Many rows:' $h $many
Check-PlaylistScrolling $h $many
Check-SelectionScrolling $h $many
Send-Command $h '&Add playlist...'
$dialog = Wait-OwnedWindow $h
Set-PlaylistLink $dialog 'https://music.youtube.com/playlist?list=PLcheck2'
Check 'A kept playlist stored without a name is looked up' (Wait-LinkLine $dialog) 'Checked PLcheck2 YouTube Music playlist | Already added False'
[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Wait-Closed $dialog
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-many.png") }
Stop-App $app

"--- Run 4: state.json doesn't read"
Check-BrokenState
