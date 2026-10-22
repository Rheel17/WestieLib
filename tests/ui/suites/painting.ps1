# Painting during splitter drags: no window keeps an unpainted area after a drag step, and nothing blinks on screen.
# The blink checks watch the screen, so a click in another window during a run spoils them; run them again without
# touching the mouse. Dot-sourced by ui-checks.ps1.

# Splitter drags don't make text blink on screen: the left panel's content stays exactly as it was while the playlists
# splitter moves, and the inspector's placeholder and the welcome screen, which move with their panels, never lose
# their text. The welcome drag stays short enough for its column to keep its width, so its text wraps the same.
function Check-NoBlinking($hwnd) {
	$welcome = [W]::RectIn((Get-Panel $hwnd 'Welcome'), $hwnd)
	$texts = @(Get-DirectChildren (Get-Panel $hwnd 'Welcome') 'WestieLib.Label' | ForEach-Object { [W]::RectIn($_, $hwnd) })
	$column = New-Object W+RECT
	$column.Left = $welcome.Left
	$column.Right = $welcome.Right
	$column.Top = ($texts | Measure-Object -Property Top -Minimum).Minimum - (S 52)
	$column.Bottom = [W]::RectIn((Get-DirectChildren (Get-Panel $hwnd 'Welcome') 'Button')[0], $hwnd).Bottom
	Check 'Playlists drag: welcome screen never blinks' ([W]::WatchDrag($hwnd, $column, (Get-Splitter $hwnd 0), (S 200), $true)) 'blinked 0'

	$panel = (Get-RowPanels $hwnd)[0]
	$button = [W]::RectIn(([W]::Children($panel, 'Button'))[0], $hwnd)
	$left = New-Object W+RECT
	$left.Right = S 150
	$left.Top = Get-RowTop $hwnd
	$left.Bottom = $button.Bottom + (S 8)
	Check 'Playlists drag: left panel never blinks' ([W]::WatchDrag($hwnd, $left, (Get-Splitter $hwnd 0), (S 300), $false)) 'blinked 0'

	$inspector = [W]::RectIn((Get-RowPanels $hwnd)[2], $hwnd)
	$placeholder = [W]::RectIn((Get-InspectorPlaceholder $hwnd), $hwnd)
	$band = New-Object W+RECT
	$band.Left = $inspector.Left - (S 200)
	$band.Right = $inspector.Right
	$band.Top = $placeholder.Top
	$band.Bottom = $placeholder.Bottom
	Check 'Inspector drag: placeholder never blinks' ([W]::WatchDrag($hwnd, $band, (Get-Splitter $hwnd 1), -(S 200), $true)) 'blinked 0'
}

# Counts drag steps after which some window still has an unpainted area
function Measure-UnpaintedDragSteps($hwnd) {
	$windows = @($hwnd) + @([W]::Children($hwnd, $null) | Where-Object { [W]::IsWindowVisible($_) })
	$splitter = Get-Splitter $hwnd 0
	$unpainted = 0
	[void][W]::SendMessageW($splitter, 0x201, [IntPtr]1, (LParam 2 10))
	foreach ($left in (@((S 260)..(S 560)) + @((S 560)..(S 260)))) {
		$current = [W]::RectIn($splitter, $hwnd).Left
		[void][W]::SendMessageW($splitter, 0x200, [IntPtr]1, (LParam ($left - $current + 2) 10))
		if ($windows | Where-Object { $r = New-Object W+RECT; [W]::GetUpdateRect($_, [ref]$r, $false) }) { $unpainted++ }
	}
	[void][W]::SendMessageW($splitter, 0x202, [IntPtr]0, (LParam 2 10))
	$unpainted
}

"--- Run 1: the welcome screen"
$app = Start-App
$h = $app.Hwnd
Check 'Drag steps left unpainted' (Measure-UnpaintedDragSteps $h) 0
Check-NoBlinking $h
Stop-App $app

"--- Run 2: more playlists than fit"
Write-ManyPlaylists 20
Set-SavedWindow @{ InspectorWidthDips = 500 }
$app = Start-App
$h = $app.Hwnd
# Wide enough for the main column to give the inspector drag room above its 400. The bounds are set here rather than
# saved, because they scale with the window's DPI.
Set-Bounds $h 100 100 (S 1600) (S 750)
Start-Sleep -Milliseconds 500
$left = New-Object W+RECT
$left.Right = S 150
$left.Top = Get-RowTop $h
$left.Bottom = [W]::RectIn((Get-RowPanels $h)[0], $h).Bottom
Check 'Playlists drag: rows never blink' ([W]::WatchDrag($h, $left, (Get-Splitter $h 0), (S 300), $false)) 'blinked 0'
$card = [W]::RectIn((Get-InspectorCard $h), $h)
$band = New-Object W+RECT
$band.Left = $card.Left - (S 200)
$band.Right = [W]::RectIn((Get-Panel $h 'Inspector'), $h).Right
$band.Top = $card.Top
$band.Bottom = $card.Bottom
# The main column gives the inspector only what it has above its 400
$room = (Get-ClientWidth (Get-RowPanels $h)[1]) - (S 400)
Check 'Inspector drag: Library card never blinks' ([W]::WatchDrag($h, $band, (Get-Splitter $h 1), -[math]::Min((S 200), $room), $true)) 'blinked 0'
Stop-App $app
