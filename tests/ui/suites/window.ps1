# The main window: DPI awareness, icons, menus and their states, commands and shortcuts, the toolbar and status bar,
# the side panels' first-run content, sizes, splitter drags and limits, the log pane, and saving and restoring the
# window state across restarts. Dot-sourced by ui-checks.ps1.

# Drags splitter 0 (playlists) or 1 (inspector) so its left edge lands on $left
function Move-Splitter($hwnd, [int]$index, [int]$left) {
	$splitter = Get-Splitter $hwnd $index
	$r = [W]::RectIn($splitter, $hwnd)
	[void][W]::SendMessageW($splitter, 0x201, [IntPtr]1, (LParam 2 10))
	[void][W]::SendMessageW($splitter, 0x200, [IntPtr]1, (LParam ($left - $r.Left + 2) 10))
	[void][W]::SendMessageW($splitter, 0x202, [IntPtr]0, (LParam ($left - $r.Left + 2) 10))
}

# Drags the log splitter so its top edge lands on $top
function Move-LogSplitter($hwnd, [int]$top) {
	$splitter = Get-LogSplitter $hwnd
	$r = [W]::RectIn($splitter, $hwnd)
	[void][W]::SendMessageW($splitter, 0x201, [IntPtr]1, (LParam 10 2))
	[void][W]::SendMessageW($splitter, 0x200, [IntPtr]1, (LParam 10 ($top - $r.Top + 2)))
	[void][W]::SendMessageW($splitter, 0x202, [IntPtr]0, (LParam 10 ($top - $r.Top + 2)))
}

# The log and its splitter span the window right above the status bar, the log $height tall
function Check-Log([string]$what, $hwnd, [int]$height) {
	$cw = Get-ClientWidth $hwnd
	$bottom = [W]::RectIn((Get-StatusBar $hwnd), $hwnd).Top
	$g = S 5
	$actual = "$(Format-Rect ([W]::RectIn((Get-LogSplitter $hwnd), $hwnd))) | $(Format-Rect ([W]::RectIn((Get-LogPanel $hwnd), $hwnd))) $(Get-LogVisibility $hwnd)"
	Check $what $actual "0,$($bottom - $height - $g) $($cw)x$g | 0,$($bottom - $height) $($cw)x$height True True"
}

# The log's height while the panels keep their 240 minimum
function Get-TallestLog($hwnd) { (Get-ClientHeight $hwnd) - (S 34) - (S 23) - (S 5) - (S 240) }

"--- Run 1: no saved state"
$app = Start-App
$h = $app.Hwnd
"DPI $script:dpi"
Check 'DPI awareness (2 = per monitor)' ([W]::GetAwarenessFromDpiAwarenessContext([W]::GetWindowDpiAwarenessContext($h))) 2
Check 'Big and small icons set' "$([W]::SendMessageW($h, 0x7F, [IntPtr]1, [IntPtr]::Zero) -ne 0) $([W]::SendMessageW($h, 0x7F, [IntPtr]0, [IntPtr]::Zero) -ne 0)" 'True True'
$menu = [W]::GetMenu($h)
Check 'Menu bar' ([W]::MenuText($menu)) '&File &Update &Track &View T&ools &Help'
$popups = @(
	'&Add playlist... [Ctrl+N], &Remove playlist..., -, &Open library folder, -, E&xit [Ctrl+Q]',
	'Update &all [F5], Update &selected playlist, -, &Cancel',
	'&Confirm match, C&hoose match..., &Edit track... [Enter], -, &Retry, &Keep, &Delete file..., -, &Show in folder, Show in &log, Open on &YouTube Music, Open on S&potify',
	'&Log [Ctrl+L], -, &Columns...',
	'&Settings... [Ctrl+Alt+S], Check for &updates, -, &Rebuild library from files..., Locate library &folder..., -, Open &log folder',
	'&About WestieLib'
)
for ($i = 0; $i -lt $popups.Count; $i++) {
	Check "  $(([W]::MenuText($menu) -split ' ')[$i]) items" ([W]::PopupText([W]::GetSubMenu($menu, $i))) $popups[$i]
}
Update-MenuStates $h
Check 'Grayed while idle, no playlists, nothing selected' ([W]::ItemsWithState($menu, 0x1)) ('&Remove playlist..., Update &all, Update &selected playlist, &Cancel, ' +
	'&Confirm match, C&hoose match..., &Edit track..., &Retry, &Keep, &Delete file..., &Show in folder, Show in &log, Open on &YouTube Music, Open on S&potify')
Check 'Checked items' ([W]::ItemsWithState($menu, 0x8)) ''
Send-Command $h '&About WestieLib'
$about = Wait-OwnedWindow $h
Check 'Help > About opens a dialog' "$([W]::ClassOf($about)) $([W]::TextOf($about))" '#32770 About WestieLib'
[void][W]::PostMessageW($about, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
Start-Sleep -Milliseconds 300
Check '  and closes' ([W]::IsWindow($about)) 'False'
Check-MenuBarEdge 'Menu bar runs into the toolbar' $h -Menus
Check 'Initial window size' ((Get-Bounds $h) -replace '^.* ', '') "$(S 1280)x$(S 800)"
Check-Bars 'Toolbar strip | status bar' $h
$toolbar = ([W]::Children((Get-ToolbarStrip $h), 'ToolbarWindow32'))[0]
Check 'Toolbar buttons, no playlists' ([W]::ToolbarButtons($toolbar)) 'Add playlist, Update all v (disabled), Cancel (disabled), -, Settings'
Check-Toolbar ' ' $h
Check-StatusParts 'Status bar part edges' $h
Check-StatusTexts ' ' $h
$cw = Get-ClientWidth $h
$g = S 5
Check-Widths 'Initial widths' $h @((S 250), ($cw - (S 250) - (S 290) - 2 * $g), (S 290))
Check-PlaylistPanel 'Left panel:' $h
Check 'Inspector shows nothing' (@([W]::Children((Get-Panel $h 'Inspector'), $null) | Where-Object { [W]::IsWindowVisible($_) }).Count) 0
Check-Welcome 'Welcome screen' $h
Move-Splitter $h 0 0
Move-Splitter $h 0 (S 400)
Check-Widths 'Playlists splitter to 400' $h @((S 400), ($cw - (S 400) - (S 290) - 2 * $g), (S 290))
Move-Splitter $h 0 5000
Check-Widths 'Playlists far right: tracks stops at 400' $h @(($cw - (S 290) - 2 * $g - (S 400)), (S 400), (S 290)) 1
Move-Splitter $h 0 -100
Check-Widths 'Playlists far left: stops at 250' $h @((S 250), ($cw - (S 250) - (S 290) - 2 * $g), (S 290))
Move-Splitter $h 1 ($cw - $g - (S 500))
Check-Widths 'Inspector splitter for a 500 inspector' $h @((S 250), ($cw - (S 250) - (S 500) - 2 * $g), (S 500))
Move-Splitter $h 1 -1000
Check-Widths 'Inspector far left: tracks stops at 400' $h @((S 250), (S 400), ($cw - (S 250) - (S 400) - 2 * $g)) 1
Move-Splitter $h 1 ($cw - $g - (S 500))
Set-Bounds $h 100 100 600 400
Check 'Minimum window size' ((Get-Bounds $h) -replace '^.* ', '') "$(S 1100)x$(S 680)"
$cw = Get-ClientWidth $h
Check-Widths 'At minimum size: inspector gives way first' $h @((S 250), (S 400), ($cw - (S 250) - (S 400) - 2 * $g))
Check-Bars '  toolbar strip | status bar' $h
Check-StatusParts '  status bar part edges' $h
Check-StatusTexts ' ' $h
Set-Bounds $h 100 100 (S 1200) (S 750)
$cw = Get-ClientWidth $h
Check-Widths 'Window grows: inspector back to 500' $h @((S 250), ($cw - (S 250) - (S 500) - 2 * $g), (S 500))
Check 'Log hidden at first start' (Get-LogVisibility $h) 'False False'
Send-Command $h '&Log'
Start-Sleep -Milliseconds 200
Check-Log 'View > Log shows it above the status bar' $h (S 168)
Check-Widths '  panels above it keep their widths' $h @((S 250), ($cw - (S 250) - (S 500) - 2 * $g), (S 500))
Update-MenuStates $h
Check '  and checks View > Log' ([W]::ItemsWithState($menu, 0x8)) '&Log'

# Its toolbar: Log, Level and its list, the scope's flat buttons, Open log folder and the close button
$toolbar = Get-LogToolbar $h
$labels = @([W]::Children($toolbar, 'WestieLib.Label') | ForEach-Object { [W]::TextOf($_) }) -join ' '
Check 'Log toolbar: labels | levels | scopes | button' "$labels | $([W]::ComboItems((Get-LogLevelBox $h))) | $([W]::TabTexts((Get-LogScopeTabs $h))) | $([W]::TextOf(([W]::Children($toolbar, 'Button'))[0]))" (
	'Log Level | Trace and up | Debug and up | Info and up | Warnings and up | Errors only | All | Selected playlist | Selected track | Open log folder')
Check '  Info and up, All' (Get-LogView $h) '2 0'
$close = ([W]::Children($toolbar, 'WestieLib.CloseButton'))[0]
$title = [W]::RectIn(([W]::Children($toolbar, 'WestieLib.Label'))[0], $toolbar)
$box = [W]::RectIn($close, $toolbar)
Check '  28 tall, Log 8 from the left, the close button 24 square, 4 from the right' (
	"$(Get-ClientHeight $toolbar) $($title.Left) $($box.Right - $box.Left)x$($box.Bottom - $box.Top) $((Get-ClientWidth $toolbar) - $box.Right)") (
	"$(S 28) $(S 8) $(S 24)x$(S 24) $(S 4)")
$list = Get-LogList $h
$columns = [W]::ListViewColumns($list)
Check '  columns, Message to the edge' ($columns -replace ' \d+$', '') "Time $(S 150) | Level $(S 70) | Playlist / Track $(S 160) | Message"
Check '  Message takes the rest' ([int]($columns -replace '^.* ', '')) ((Get-ClientWidth $list) - (S 150) - (S 70) - (S 160))
Choose-LogLevel $h 4
Click-LogScope $h 2
Check 'Errors only, Selected track' (Get-LogView $h) '4 2'
Check '  no track selected: the list says so' "[$(Get-LogRows $h)] $([W]::TextOf($list))" '[] Select a track to see its lines.'
Send-Click $close (S 12) (S 12)
Start-Sleep -Milliseconds 200
Check 'The close button hides the log' (Get-LogVisibility $h) 'False False'
Update-MenuStates $h
Check '  and unchecks View > Log' "[$([W]::ItemsWithState($menu, 0x8))]" '[]'
Send-Command $h '&Log'
Start-Sleep -Milliseconds 200

Move-LogSplitter $h ((Get-ClientHeight $h) - 10)
Check-Log 'Log splitter far down: log stops at 100' $h (S 100)
Move-LogSplitter $h -100
$tallest = Get-TallestLog $h
Check-Log 'Log splitter far up: panels stop at 240' $h $tallest
Set-Bounds $h 100 100 600 400
Check-Log 'At minimum size: the log gives way' $h (Get-TallestLog $h)
Check-Widths '  panels keep 240' $h @((S 250), (S 400), ((Get-ClientWidth $h) - (S 250) - (S 400) - 2 * $g))
Set-Bounds $h 100 100 (S 1200) (S 750)
Check-Log 'Window grows: the log height comes back' $h $tallest
Move-LogSplitter $h ([W]::RectIn((Get-StatusBar $h), $h).Top - (S 200) - $g)
Check-Log 'Log splitter for a 200 log' $h (S 200)
[W]::PostCtrlKey($h, 0x4C)
Check 'Ctrl+L hides the log' (Get-LogVisibility $h) 'False False'
Check-Widths '  panels reach the status bar' $h @((S 250), ($cw - (S 250) - (S 500) - 2 * $g), (S 500))
[W]::PostCtrlKey($h, 0x4C)
Check-Log '  and shows it again' $h (S 200)
"File > Exit"
Stop-App $app -ExitCommand
$saved = Get-ItemProperty $KeyPath
Check '  saved bounds' "$($saved.Left),$($saved.Top),$($saved.Right),$($saved.Bottom)" "100,100,$(100 + (S 1200)),$(100 + (S 750))"
Check '  saved maximized, widths (DIPs)' "$($saved.Maximized) $($saved.PlaylistsWidthDips) $($saved.InspectorWidthDips)" '0 250 500'
Check '  saved log visible, height (DIPs)' "$($saved.LogVisible) $($saved.LogHeightDips)" '1 200'
Check '  saved log level, scope' "$($saved.LogLevel) $($saved.LogScope)" '4 2'

"--- Run 2: restore a normal window"
# With tools, so the screenshot shows the window as it usually looks
Install-FakeTools
Set-SavedAutoUpdate $false
$app = Start-App
$h = $app.Hwnd
$cw = Get-ClientWidth $h
Check 'Restored bounds' (Get-Bounds $h) "100,100 $(S 1200)x$(S 750)"
Check-Widths 'Restored widths' $h @((S 250), ($cw - (S 250) - (S 500) - 2 * $g), (S 500))
Check-Bars '  toolbar strip | status bar' $h
Check-Log 'Restored log' $h (S 200)
Check '  its level and scope' (Get-LogView $h) '4 2'
Choose-LogLevel $h 2
Click-LogScope $h 0
Start-Sleep -Milliseconds 200
Check '  back to All: no empty text' "[$([W]::TextOf((Get-LogList $h)))]" '[]'
if ($Screenshot) { Save-Screenshot $h $Screenshot }
Send-Command $h '&Log'
Start-Sleep -Milliseconds 200
Check 'View > Log hides it' (Get-LogVisibility $h) 'False False'
[void][W]::ShowWindow($h, 3)
Start-Sleep -Milliseconds 300
"Closed maximized"
Stop-App $app
$saved = Get-ItemProperty $KeyPath
Check '  saved maximized, log visible' "$($saved.Maximized) $($saved.LogVisible)" '1 0'

"--- Run 3: restore a maximized window"
$app = Start-App
$h = $app.Hwnd
Check 'Starts maximized' ([W]::IsZoomed($h)) 'True'
Check '  log stays hidden' (Get-LogVisibility $h) 'False False'
Check-StatusParts '  status bar part edges (no grip)' $h
Check-StatusTexts ' ' $h
[void][W]::ShowWindow($h, 9)
Start-Sleep -Milliseconds 300
Check 'Restore goes back to the saved bounds' (Get-Bounds $h) "100,100 $(S 1200)x$(S 750)"
[void][W]::ShowWindow($h, 3)
Start-Sleep -Milliseconds 300
[void][W]::ShowWindow($h, 6)
Start-Sleep -Milliseconds 300
Stop-App $app
Check '  minimized from maximized saves maximized' (Get-ItemProperty $KeyPath).Maximized 1

"--- Run 4: closed while minimized"
$app = Start-App
$h = $app.Hwnd
Check 'Comes back maximized, not minimized' "$([W]::IsZoomed($h)) $([W]::IsIconic($h))" 'True False'
Stop-App $app
