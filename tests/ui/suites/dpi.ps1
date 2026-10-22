# Everything that scales with the DPI, on a monitor with another DPI than the one the window starts on: moved there,
# which sends WM_DPICHANGED, and restored there from the saved state. Skipped when every monitor has the same DPI.
# Dot-sourced by ui-checks.ps1.

# Everything that scales with the DPI: bars, toolbar, status bar, and the default panel widths
function Check-Scaled($hwnd) {
	$cw = Get-ClientWidth $hwnd
	Check-MenuBarEdge 'Menu bar runs into the toolbar' $hwnd
	Check-Bars 'Toolbar strip | status bar' $hwnd
	Check-Toolbar ' ' $hwnd
	Check-StatusParts 'Status bar part edges' $hwnd
	Check-StatusTexts ' ' $hwnd
	Check-Widths 'Default widths' $hwnd @((S 250), ($cw - (S 250) - (S 290) - 2 * (S 5)), (S 290))
	Check-PlaylistPanel 'Left panel:' $hwnd
	Check-InspectorPlaceholder 'Inspector placeholder | sides | center offset x2' $hwnd
	Check-Welcome 'Welcome screen' $hwnd
}

"--- Run 1: moved to a monitor with another DPI"
$app = Start-App
$h = $app.Hwnd
$work = [W]::OtherDpiWorkArea($script:dpi)
if ($work.Right -eq $work.Left) {
	"Skipped: no monitor with another DPI"
	Stop-App $app
} else {
	$from = $script:dpi
	Set-Bounds $h ($work.Left + 100) ($work.Top + 100) (S 1280) (S 800)
	Start-Sleep -Milliseconds 500
	$script:dpi = [W]::GetDpiForWindow($h)
	"DPI $from to $script:dpi"
	Check-Scaled $h
	Stop-App $app

	"--- Run 2: restored onto that monitor"
	$app = Start-App
	$h = $app.Hwnd
	"DPI $script:dpi"
	Check-Scaled $h
	Stop-App $app
}
