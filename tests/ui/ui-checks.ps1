# Drives WestieLib.exe through Win32 messages and checks the main window: DPI awareness, icons, menu, sizes,
# splitter drags and limits, painting during a drag, and saving/restoring the window state across restarts.
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File tests/ui/ui-checks.ps1 -Exe <WestieLib.exe> [-Screenshot <png>]
#
# The exit code is the number of failed checks. The user's saved window state is set aside and put back afterwards.

param(
	[Parameter(Mandatory)] [string]$Exe,
	[string]$Screenshot
)

$ErrorActionPreference = 'Stop'
# A stale Visual Studio path in LIB makes Add-Type's C# compiler fail on a warning
$env:LIB = $null
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Linq;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public static class W {
	[StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
	[DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern IntPtr FindWindowW(string cls, string title);
	[DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr ctx);
	[DllImport("user32.dll")] public static extern IntPtr GetWindowDpiAwarenessContext(IntPtr h);
	[DllImport("user32.dll")] public static extern int GetAwarenessFromDpiAwarenessContext(IntPtr ctx);
	[DllImport("user32.dll")] public static extern uint GetDpiForWindow(IntPtr h);
	[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
	[DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
	[DllImport("user32.dll")] public static extern bool GetUpdateRect(IntPtr h, out RECT r, bool erase);
	[DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
	[DllImport("user32.dll")] public static extern IntPtr SendMessageW(IntPtr h, uint msg, IntPtr w, IntPtr l);
	[DllImport("user32.dll")] public static extern bool PostMessageW(IntPtr h, uint msg, IntPtr w, IntPtr l);
	[DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
	[DllImport("user32.dll")] public static extern bool IsZoomed(IntPtr h);
	[DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
	[DllImport("user32.dll")] public static extern IntPtr GetMenu(IntPtr h);
	[DllImport("user32.dll")] public static extern int GetMenuItemCount(IntPtr menu);
	[DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetMenuStringW(IntPtr menu, uint item, StringBuilder s, int n, uint flags);
	[DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetClassNameW(IntPtr h, StringBuilder s, int n);
	[DllImport("user32.dll")] public static extern int MapWindowPoints(IntPtr from, IntPtr to, ref RECT r, int count);
	[DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
	[DllImport("dwmapi.dll")] public static extern int DwmGetWindowAttribute(IntPtr h, int attr, out RECT r, int size);
	public delegate bool EnumProc(IntPtr h, IntPtr l);
	[DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumProc proc, IntPtr l);

	public static string ClassOf(IntPtr h) {
		var s = new StringBuilder(256);
		GetClassNameW(h, s, 256);
		return s.ToString();
	}

	public static RECT RectIn(IntPtr child, IntPtr parent) {
		RECT r;
		GetWindowRect(child, out r);
		MapWindowPoints(IntPtr.Zero, parent, ref r, 2);
		return r;
	}

	public static IntPtr[] Children(IntPtr parent, string cls) {
		var found = new List<IntPtr>();
		EnumChildWindows(parent, (h, l) => { if (cls == null || ClassOf(h) == cls) found.Add(h); return true; }, IntPtr.Zero);
		return found.OrderBy(h => RectIn(h, parent).Left).ToArray();
	}

	public static string MenuText(IntPtr menu) {
		var parts = new List<string>();
		for (uint i = 0; i < GetMenuItemCount(menu); i++) {
			var s = new StringBuilder(64);
			GetMenuStringW(menu, i, s, 64, 0x400);
			parts.Add(s.ToString());
		}
		return String.Join(" ", parts);
	}
}
'@

if (Get-Process WestieLib -ErrorAction SilentlyContinue) { throw 'WestieLib is running; close it before running the checks' }

# Physical pixels, not coordinates virtualized for a DPI-unaware PowerShell
[void][W]::SetThreadDpiAwarenessContext([IntPtr](-4))

$KeyPath = 'HKCU:\Software\WestieLib\Window'
$StateNames = 'Left', 'Top', 'Right', 'Bottom', 'Maximized', 'PlaylistsWidthDips', 'InspectorWidthDips'
$script:failures = 0
$script:dpi = 96
$app = $null

function Check([string]$what, $actual, $expected) {
	$ok = "$actual" -eq "$expected"
	if (-not $ok) { $script:failures++ }
	"{0,-56} {1,-26} {2}" -f $what, $actual, $(if ($ok) { 'ok' } else { "FAIL (expected $expected)" })
}

# DIPs to pixels, rounding like MulDiv
function S([int]$dips) { [int][math]::Floor($dips * $script:dpi / 96 + 0.5) }

function Start-App {
	$process = Start-Process -FilePath $Exe -PassThru
	for ($i = 0; $i -lt 50; $i++) {
		Start-Sleep -Milliseconds 100
		$hwnd = [W]::FindWindowW('WestieLib.MainWindow', [NullString]::Value)
		if ($hwnd -ne [IntPtr]::Zero) { break }
	}
	if ($hwnd -eq [IntPtr]::Zero) { throw 'Main window not found' }
	Start-Sleep -Milliseconds 400
	$script:dpi = [W]::GetDpiForWindow($hwnd)
	return @{ Process = $process; Hwnd = $hwnd }
}

function Stop-App($app) {
	[void][W]::PostMessageW($app.Hwnd, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
	if (-not $app.Process.WaitForExit(5000)) { throw 'Process did not exit after WM_CLOSE' }
	Check '  exit code' $app.Process.ExitCode 0
}

function Get-ClientWidth($hwnd) {
	$c = New-Object W+RECT
	[void][W]::GetClientRect($hwnd, [ref]$c)
	$c.Right
}

# Checks playlists | tracks | inspector widths (within $tolerance px), and that the splitters fill the gaps exactly
function Check-Widths([string]$what, $hwnd, [int[]]$expected, [int]$tolerance = 0) {
	$panels = @([W]::Children($hwnd, 'WestieLib.Panel') | ForEach-Object { [W]::RectIn($_, $hwnd) })
	$splitters = @([W]::Children($hwnd, 'WestieLib.Splitter') | ForEach-Object { [W]::RectIn($_, $hwnd) })
	$contiguous = $panels[0].Left -eq 0 -and $panels[0].Right -eq $splitters[0].Left -and $splitters[0].Right -eq $panels[1].Left -and
		$panels[1].Right -eq $splitters[1].Left -and $splitters[1].Right -eq $panels[2].Left -and $panels[2].Right -eq (Get-ClientWidth $hwnd)
	$widths = @($panels | ForEach-Object { $_.Right - $_.Left })
	$near = $contiguous
	for ($i = 0; $i -lt 3; $i++) { if ([math]::Abs($widths[$i] - $expected[$i]) -gt $tolerance) { $near = $false } }
	$actual = "$($widths -join ' | ')$(if (-not $contiguous) { ' (gaps!)' })"
	if (-not $near) { $script:failures++ }
	"{0,-56} {1,-26} {2}" -f $what, $actual, $(if ($near) { 'ok' } else { "FAIL (expected $($expected -join ' | '))" })
}

function LParam([int]$x, [int]$y) { [IntPtr](($y -band 0xFFFF) * 65536 + ($x -band 0xFFFF)) }

function Get-Splitter($hwnd, [int]$index) { ([W]::Children($hwnd, 'WestieLib.Splitter'))[$index] }

# Drags splitter 0 (playlists) or 1 (inspector) so its left edge lands on $left
function Move-Splitter($hwnd, [int]$index, [int]$left) {
	$splitter = Get-Splitter $hwnd $index
	$r = [W]::RectIn($splitter, $hwnd)
	[void][W]::SendMessageW($splitter, 0x201, [IntPtr]1, (LParam 2 10))
	[void][W]::SendMessageW($splitter, 0x200, [IntPtr]1, (LParam ($left - $r.Left + 2) 10))
	[void][W]::SendMessageW($splitter, 0x202, [IntPtr]0, (LParam ($left - $r.Left + 2) 10))
}

function Set-Bounds($hwnd, $x, $y, $w, $h) {
	[void][W]::SetWindowPos($hwnd, [IntPtr]::Zero, $x, $y, $w, $h, 0x4 -bor 0x10)
}

function Get-Bounds($hwnd) {
	$r = New-Object W+RECT
	[void][W]::GetWindowRect($hwnd, [ref]$r)
	"$($r.Left),$($r.Top) $($r.Right - $r.Left)x$($r.Bottom - $r.Top)"
}

function Save-Screenshot($hwnd, $path) {
	$r = New-Object W+RECT
	[void][W]::GetWindowRect($hwnd, [ref]$r)
	$frame = New-Object W+RECT
	[void][W]::DwmGetWindowAttribute($hwnd, 9, [ref]$frame, 16)
	$bitmap = New-Object System.Drawing.Bitmap ($r.Right - $r.Left), ($r.Bottom - $r.Top)
	$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
	$hdc = $graphics.GetHdc()
	[void][W]::PrintWindow($hwnd, $hdc, 2)
	$graphics.ReleaseHdc($hdc)
	$graphics.Dispose()
	$crop = New-Object System.Drawing.Rectangle ($frame.Left - $r.Left), ($frame.Top - $r.Top), ($frame.Right - $frame.Left), ($frame.Bottom - $frame.Top)
	$cropped = $bitmap.Clone($crop, $bitmap.PixelFormat)
	$cropped.Save($path)
	$cropped.Dispose()
	$bitmap.Dispose()
	"Screenshot: $path"
}

# Counts drag steps after which some window still has an unpainted area
function Measure-UnpaintedDragSteps($hwnd) {
	$windows = @($hwnd) + @([W]::Children($hwnd, $null))
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

$userState = if (Test-Path $KeyPath) { Get-ItemProperty $KeyPath } else { $null }
Remove-Item 'HKCU:\Software\WestieLib' -Recurse -Confirm:$false -ErrorAction SilentlyContinue

try {
	"--- Run 1: no saved state"
	$app = Start-App
	$h = $app.Hwnd
	"DPI $script:dpi"
	Check 'DPI awareness (2 = per monitor)' ([W]::GetAwarenessFromDpiAwarenessContext([W]::GetWindowDpiAwarenessContext($h))) 2
	Check 'Big and small icons set' "$([W]::SendMessageW($h, 0x7F, [IntPtr]1, [IntPtr]::Zero) -ne 0) $([W]::SendMessageW($h, 0x7F, [IntPtr]0, [IntPtr]::Zero) -ne 0)" 'True True'
	Check 'Menu bar' ([W]::MenuText([W]::GetMenu($h))) '&File &Sync &Track &View &Tools &Help'
	Check 'Initial window size' ((Get-Bounds $h) -replace '^.* ', '') "$(S 1280)x$(S 800)"
	$cw = Get-ClientWidth $h
	$g = S 5
	Check-Widths 'Initial widths' $h @((S 250), ($cw - (S 250) - (S 290) - 2 * $g), (S 290))
	Check 'Drag steps left unpainted' (Measure-UnpaintedDragSteps $h) 0
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
	Set-Bounds $h 100 100 (S 1200) (S 750)
	$cw = Get-ClientWidth $h
	Check-Widths 'Window grows: inspector back to 500' $h @((S 250), ($cw - (S 250) - (S 500) - 2 * $g), (S 500))
	Stop-App $app
	$saved = Get-ItemProperty $KeyPath
	Check '  saved bounds' "$($saved.Left),$($saved.Top),$($saved.Right),$($saved.Bottom)" "100,100,$(100 + (S 1200)),$(100 + (S 750))"
	Check '  saved maximized, widths (DIPs)' "$($saved.Maximized) $($saved.PlaylistsWidthDips) $($saved.InspectorWidthDips)" '0 250 500'

	"--- Run 2: restore a normal window"
	$app = Start-App
	$h = $app.Hwnd
	$cw = Get-ClientWidth $h
	Check 'Restored bounds' (Get-Bounds $h) "100,100 $(S 1200)x$(S 750)"
	Check-Widths 'Restored widths' $h @((S 250), ($cw - (S 250) - (S 500) - 2 * $g), (S 500))
	if ($Screenshot) { Save-Screenshot $h $Screenshot }
	[void][W]::ShowWindow($h, 3)
	Start-Sleep -Milliseconds 300
	Stop-App $app
	Check '  saved maximized' (Get-ItemProperty $KeyPath).Maximized 1

	"--- Run 3: restore a maximized window"
	$app = Start-App
	$h = $app.Hwnd
	Check 'Starts maximized' ([W]::IsZoomed($h)) 'True'
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
} finally {
	if ($app -and -not $app.Process.HasExited) { $app.Process.Kill() }
	Remove-Item 'HKCU:\Software\WestieLib' -Recurse -Confirm:$false -ErrorAction SilentlyContinue
	if ($userState) {
		New-Item $KeyPath -Force | Out-Null
		foreach ($name in $StateNames) {
			if ($null -ne $userState.$name) { New-ItemProperty $KeyPath -Name $name -PropertyType DWord -Value $userState.$name | Out-Null }
		}
		"Put the user's saved window state back"
	}
}

"--- $($script:failures) failure(s)"
exit $script:failures
