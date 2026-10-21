# Drives WestieLib.exe through Win32 messages and checks the main window: DPI awareness, icons, menus and their
# states, commands and shortcuts, the toolbar and status bar, sizes, splitter drags and limits, the log pane, painting
# during a drag, and saving/restoring the window state across restarts.
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
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
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
	[DllImport("user32.dll")] public static extern IntPtr GetSubMenu(IntPtr menu, int pos);
	[DllImport("user32.dll")] public static extern int GetMenuItemCount(IntPtr menu);
	[DllImport("user32.dll")] public static extern uint GetMenuItemID(IntPtr menu, int pos);
	[DllImport("user32.dll")] public static extern uint GetMenuState(IntPtr menu, uint item, uint flags);
	[DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetMenuStringW(IntPtr menu, uint item, StringBuilder s, int n, uint flags);
	[DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetClassNameW(IntPtr h, StringBuilder s, int n);
	[DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetWindowTextW(IntPtr h, StringBuilder s, int n);
	[DllImport("user32.dll")] public static extern IntPtr GetWindow(IntPtr h, uint cmd);
	[DllImport("user32.dll")] public static extern bool IsWindow(IntPtr h);
	[DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
	[DllImport("user32.dll")] public static extern int MapWindowPoints(IntPtr from, IntPtr to, ref RECT r, int count);
	[DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
	[DllImport("dwmapi.dll")] public static extern int DwmGetWindowAttribute(IntPtr h, int attr, out RECT r, int size);
	public delegate bool EnumProc(IntPtr h, IntPtr l);
	[DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumProc proc, IntPtr l);
	[DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc proc, IntPtr l);
	[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out int pid);
	[DllImport("kernel32.dll")] public static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
	[DllImport("kernel32.dll")] public static extern IntPtr VirtualAllocEx(IntPtr process, IntPtr address, IntPtr size, uint type, uint protect);
	[DllImport("kernel32.dll")] public static extern bool VirtualFreeEx(IntPtr process, IntPtr address, IntPtr size, uint type);
	[DllImport("kernel32.dll")] public static extern bool ReadProcessMemory(IntPtr process, IntPtr address, byte[] buffer, IntPtr size, out IntPtr read);
	[DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr h);
	[DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
	[DllImport("user32.dll")] public static extern bool AttachThreadInput(uint attach, uint to, bool on);
	[DllImport("user32.dll")] public static extern bool GetKeyboardState(byte[] state);
	[DllImport("user32.dll")] public static extern bool SetKeyboardState(byte[] state);

	// Presses Ctrl+key in the app without real input, so nothing can reach the user's other windows. While the threads
	// are attached they share their key state, so the app's TranslateAccelerator sees Ctrl held.
	public static void PostCtrlKey(IntPtr h, int key) {
		int pid;
		uint thread = GetWindowThreadProcessId(h, out pid), self = GetCurrentThreadId();
		AttachThreadInput(self, thread, true);
		var state = new byte[256];
		GetKeyboardState(state);
		var saved = (byte[])state.Clone();
		try {
			state[0x11] |= 0x80;
			SetKeyboardState(state);
			PostMessageW(h, 0x100, (IntPtr)key, (IntPtr)1);
			System.Threading.Thread.Sleep(300);
		} finally {
			SetKeyboardState(saved);
			AttachThreadInput(self, thread, false);
		}
	}

	// Sends a message whose lparam is a buffer the window fills. Common controls don't marshal pointers across
	// processes, so the buffer lives in the window's process.
	public static byte[] SendForBuffer(IntPtr h, uint msg, IntPtr w, int size, out long result) {
		int pid;
		GetWindowThreadProcessId(h, out pid);
		IntPtr process = OpenProcess(0x38, false, pid);
		IntPtr remote = VirtualAllocEx(process, IntPtr.Zero, (IntPtr)size, 0x3000, 0x04);
		try {
			result = (long)SendMessageW(h, msg, w, remote);
			var buffer = new byte[size];
			IntPtr read;
			ReadProcessMemory(process, remote, buffer, (IntPtr)size, out read);
			return buffer;
		} finally {
			VirtualFreeEx(process, remote, IntPtr.Zero, 0x8000);
			CloseHandle(process);
		}
	}

	static string Utf16(byte[] bytes) {
		var s = Encoding.Unicode.GetString(bytes);
		int end = s.IndexOf('\0');
		return end < 0 ? s : s.Substring(0, end);
	}

	// Button texts, with " v" for a dropdown and " (disabled)"; separators show as "-"
	public static string ToolbarButtons(IntPtr toolbar) {
		var parts = new List<string>();
		long result;
		int count = (int)SendMessageW(toolbar, 0x418, IntPtr.Zero, IntPtr.Zero);
		for (int i = 0; i < count; i++) {
			// TBBUTTON on x64: idCommand at 4, fsState at 8, fsStyle at 9
			var button = SendForBuffer(toolbar, 0x417, (IntPtr)i, 32, out result);
			if ((button[9] & 0x1) != 0) { parts.Add("-"); continue; }
			var text = Utf16(SendForBuffer(toolbar, 0x44B, (IntPtr)BitConverter.ToInt32(button, 4), 256, out result));
			parts.Add(text + ((button[9] & 0x8) != 0 ? " v" : "") + ((button[8] & 0x4) == 0 ? " (disabled)" : ""));
		}
		return String.Join(", ", parts);
	}

	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
	public struct LOGFONTW {
		public int Height, Width, Escapement, Orientation, Weight;
		public byte Italic, Underline, StrikeOut, CharacterSet, OutPrecision, ClipPrecision, Quality, PitchAndFamily;
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string FaceName;
	}
	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
	public struct NONCLIENTMETRICSW {
		public int Size, BorderWidth, ScrollWidth, ScrollHeight, CaptionWidth, CaptionHeight;
		public LOGFONTW CaptionFont;
		public int SmCaptionWidth, SmCaptionHeight;
		public LOGFONTW SmCaptionFont;
		public int MenuWidth, MenuHeight;
		public LOGFONTW MenuFont, StatusFont, MessageFont;
		public int PaddedBorderWidth;
	}
	[StructLayout(LayoutKind.Sequential)] public struct SIZE { public int Cx, Cy; }
	[DllImport("user32.dll")] public static extern bool SystemParametersInfoForDpi(uint action, uint param, ref NONCLIENTMETRICSW value, uint winIni, uint dpi);
	[DllImport("gdi32.dll", CharSet = CharSet.Unicode)] public static extern IntPtr CreateFontIndirectW(ref LOGFONTW font);
	[DllImport("gdi32.dll")] public static extern IntPtr CreateCompatibleDC(IntPtr dc);
	[DllImport("gdi32.dll")] public static extern IntPtr SelectObject(IntPtr dc, IntPtr obj);
	[DllImport("gdi32.dll")] public static extern bool DeleteObject(IntPtr obj);
	[DllImport("gdi32.dll")] public static extern bool DeleteDC(IntPtr dc);
	[DllImport("gdi32.dll", CharSet = CharSet.Unicode)] public static extern bool GetTextExtentPoint32W(IntPtr dc, string text, int length, out SIZE size);

	// The width of text in the message font at a DPI, the font the app gives its controls
	public static int TextWidth(string text, uint dpi) {
		var metrics = new NONCLIENTMETRICSW();
		metrics.Size = Marshal.SizeOf(metrics);
		SystemParametersInfoForDpi(0x29, (uint)metrics.Size, ref metrics, 0, dpi);
		IntPtr font = CreateFontIndirectW(ref metrics.MessageFont), dc = CreateCompatibleDC(IntPtr.Zero);
		try {
			SelectObject(dc, font);
			SIZE size;
			GetTextExtentPoint32W(dc, text, text.Length, out size);
			return size.Cx;
		} finally {
			DeleteDC(dc);
			DeleteObject(font);
		}
	}

	// "<text> +<width beyond the text>" for each plain button; separators and dropdowns are left out
	public static string ToolbarExtraWidths(IntPtr toolbar, uint dpi) {
		var parts = new List<string>();
		long result;
		int count = (int)SendMessageW(toolbar, 0x418, IntPtr.Zero, IntPtr.Zero);
		for (int i = 0; i < count; i++) {
			var button = SendForBuffer(toolbar, 0x417, (IntPtr)i, 32, out result);
			if ((button[9] & 0x9) != 0) continue;
			var text = Utf16(SendForBuffer(toolbar, 0x44B, (IntPtr)BitConverter.ToInt32(button, 4), 256, out result));
			var rect = SendForBuffer(toolbar, 0x41D, (IntPtr)i, 16, out result);
			parts.Add(text + " +" + (BitConverter.ToInt32(rect, 8) - BitConverter.ToInt32(rect, 0) - TextWidth(text, dpi)));
		}
		return String.Join(", ", parts);
	}

	[StructLayout(LayoutKind.Sequential)] public struct MONITORINFO { public int Size; public RECT Monitor, Work; public uint Flags; }
	public delegate bool MonitorProc(IntPtr monitor, IntPtr dc, ref RECT rect, IntPtr data);
	[DllImport("user32.dll")] public static extern bool EnumDisplayMonitors(IntPtr dc, IntPtr clip, MonitorProc proc, IntPtr data);
	[DllImport("user32.dll")] public static extern bool GetMonitorInfoW(IntPtr monitor, ref MONITORINFO info);
	[DllImport("shcore.dll")] public static extern int GetDpiForMonitor(IntPtr monitor, int type, out uint x, out uint y);

	// The work area of a monitor whose DPI isn't dpi, or an empty rect when there's none
	public static RECT OtherDpiWorkArea(uint dpi) {
		RECT found = new RECT();
		EnumDisplayMonitors(IntPtr.Zero, IntPtr.Zero, (IntPtr monitor, IntPtr dc, ref RECT rect, IntPtr data) => {
			uint x, y;
			GetDpiForMonitor(monitor, 0, out x, out y);
			if (x == dpi) return true;
			var info = new MONITORINFO();
			info.Size = Marshal.SizeOf(info);
			GetMonitorInfoW(monitor, ref info);
			found = info.Work;
			return false;
		}, IntPtr.Zero);
		return found;
	}

	// "gaps <between each pair of buttons> | separator <width>"
	public static string ToolbarSpacing(IntPtr toolbar) {
		var gaps = new List<string>();
		long result;
		int separator = 0, previousRight = -1;
		int count = (int)SendMessageW(toolbar, 0x418, IntPtr.Zero, IntPtr.Zero);
		for (int i = 0; i < count; i++) {
			var rect = SendForBuffer(toolbar, 0x41D, (IntPtr)i, 16, out result);
			int left = BitConverter.ToInt32(rect, 0), right = BitConverter.ToInt32(rect, 8);
			if (previousRight >= 0) gaps.Add((left - previousRight).ToString());
			previousRight = right;
			if ((SendForBuffer(toolbar, 0x417, (IntPtr)i, 32, out result)[9] & 0x1) != 0) separator = right - left;
		}
		return "gaps " + String.Join(" ", gaps) + " | separator " + separator;
	}

	// The right edges of the parts
	public static string StatusParts(IntPtr bar) {
		long count;
		var edges = SendForBuffer(bar, 0x406, (IntPtr)16, 64, out count);
		var parts = new List<string>();
		for (int i = 0; i < count; i++) parts.Add(BitConverter.ToInt32(edges, i * 4).ToString());
		return String.Join(" ", parts);
	}

	public static RECT StatusPartRect(IntPtr bar, int part) {
		long result;
		var b = SendForBuffer(bar, 0x40A, (IntPtr)part, 16, out result);
		return new RECT { Left = BitConverter.ToInt32(b, 0), Top = BitConverter.ToInt32(b, 4), Right = BitConverter.ToInt32(b, 8), Bottom = BitConverter.ToInt32(b, 12) };
	}

	// Captures the window with PrintWindow, as for the screenshot; (dx, dy) is where the child's client area starts in it
	static System.Drawing.Bitmap Capture(IntPtr window, IntPtr child, out int dx, out int dy) {
		RECT frame;
		GetWindowRect(window, out frame);
		RECT origin = new RECT();
		MapWindowPoints(child, IntPtr.Zero, ref origin, 1);
		dx = origin.Left - frame.Left;
		dy = origin.Top - frame.Top;
		var bitmap = new System.Drawing.Bitmap(frame.Right - frame.Left, frame.Bottom - frame.Top);
		using (var graphics = System.Drawing.Graphics.FromImage(bitmap)) {
			IntPtr dc = graphics.GetHdc();
			PrintWindow(window, dc, 2);
			graphics.ReleaseHdc(dc);
		}
		return bitmap;
	}

	static int Brightness(System.Drawing.Color p) { return (p.R + p.G + p.B) / 3; }

	// The gaps between a status bar part's left and right edges and its text, found as the columns with pixels darker
	// than mid-gray
	public static int[] StatusTextGaps(IntPtr window, IntPtr bar, int part) {
		RECT area = StatusPartRect(bar, part);
		int dx, dy;
		using (var bitmap = Capture(window, bar, out dx, out dy)) {
			int first = -1, last = -1;
			for (int x = area.Left; x < area.Right; x++) {
				for (int y = area.Top; y < area.Bottom; y++) {
					if (Brightness(bitmap.GetPixel(x + dx, y + dy)) < 128) { if (first < 0) first = x; last = x; break; }
				}
			}
			return new[] { first - area.Left, area.Right - 1 - last };
		}
	}

	// The bounds of a toolbar button's text (the pixels that aren't white, textOffset into the button), and whether its
	// edges have ClearType's colored fringes
	public static string ToolbarTextInk(IntPtr window, IntPtr toolbar, int index, int textOffset) {
		long result;
		var item = SendForBuffer(toolbar, 0x41D, (IntPtr)index, 16, out result);
		int left = BitConverter.ToInt32(item, 0) + textOffset, top = BitConverter.ToInt32(item, 4);
		int right = BitConverter.ToInt32(item, 8), bottom = BitConverter.ToInt32(item, 12);
		int dx, dy;
		using (var bitmap = Capture(window, toolbar, out dx, out dy)) {
			int x0 = int.MaxValue, y0 = int.MaxValue, x1 = -1, y1 = -1;
			bool colored = false;
			for (int x = left; x < right; x++) {
				for (int y = top; y < bottom; y++) {
					var p = bitmap.GetPixel(x + dx, y + dy);
					if (p.R < 0xF0 || p.G < 0xF0 || p.B < 0xF0) { x0 = Math.Min(x0, x); y0 = Math.Min(y0, y); x1 = Math.Max(x1, x); y1 = Math.Max(y1, y); }
					if (Math.Abs(p.R - p.B) > 12) colored = true;
				}
			}
			return x0 + "," + y0 + ".." + x1 + "," + y1 + (colored ? " ClearType" : " grayscale");
		}
	}

	// The x of each separator line in the status bar: a run of columns darker than the bar's face over at least 80% of
	// a part's height. The grip's dots and text don't reach that.
	public static int[] StatusSeparators(IntPtr window, IntPtr bar) {
		RECT client, part = StatusPartRect(bar, 0);
		GetClientRect(bar, out client);
		var lines = new List<int>();
		int dx, dy;
		using (var bitmap = Capture(window, bar, out dx, out dy)) {
			bool previous = false;
			for (int x = 0; x < client.Right; x++) {
				int dark = 0;
				for (int y = part.Top; y < part.Bottom; y++) {
					if (Brightness(bitmap.GetPixel(x + dx, y + dy)) < 0xE8) dark++;
				}
				bool line = dark * 10 >= (part.Bottom - part.Top) * 8;
				if (line && !previous) lines.Add(x);
				previous = line;
			}
		}
		return lines.ToArray();
	}

	public static string ClassOf(IntPtr h) {
		var s = new StringBuilder(256);
		GetClassNameW(h, s, 256);
		return s.ToString();
	}

	public static string TextOf(IntPtr h) {
		var s = new StringBuilder(256);
		GetWindowTextW(h, s, 256);
		return s.ToString();
	}

	public static IntPtr OwnedWindow(IntPtr owner) {
		IntPtr found = IntPtr.Zero;
		EnumWindows((h, l) => { if (GetWindow(h, 4) == owner && IsWindowVisible(h)) { found = h; return false; } return true; }, IntPtr.Zero);
		return found;
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

	// The item's text with "..." for the ellipsis, and its shortcut in brackets
	public static string ItemText(IntPtr popup, int pos) {
		var s = new StringBuilder(128);
		GetMenuStringW(popup, (uint)pos, s, 128, 0x400);
		var parts = s.ToString().Replace(new string((char)0x2026, 1), "...").Split('\t');
		return parts.Length > 1 ? parts[0] + " [" + parts[1] + "]" : parts[0];
	}

	// Separators show as "-"
	public static string PopupText(IntPtr popup) {
		var parts = new List<string>();
		for (int i = 0; i < GetMenuItemCount(popup); i++) parts.Add(GetMenuItemID(popup, i) == 0 ? "-" : ItemText(popup, i));
		return String.Join(", ", parts);
	}

	// Items of every popup whose state has all the given MF_ flags, without their shortcuts
	public static string ItemsWithState(IntPtr menuBar, uint flags) {
		var parts = new List<string>();
		for (int m = 0; m < GetMenuItemCount(menuBar); m++) {
			var popup = GetSubMenu(menuBar, m);
			for (int i = 0; i < GetMenuItemCount(popup); i++) {
				if (GetMenuItemID(popup, i) != 0 && (GetMenuState(popup, (uint)i, 0x400) & flags) == flags) parts.Add(ItemText(popup, i).Split('[')[0].Trim());
			}
		}
		return String.Join(", ", parts);
	}

	public static uint CommandOf(IntPtr menuBar, string text) {
		for (int m = 0; m < GetMenuItemCount(menuBar); m++) {
			var popup = GetSubMenu(menuBar, m);
			for (int i = 0; i < GetMenuItemCount(popup); i++) {
				if (ItemText(popup, i).Split('[')[0].Trim() == text) return GetMenuItemID(popup, i);
			}
		}
		throw new ArgumentException("No menu item " + text);
	}
}
'@

if (Get-Process WestieLib -ErrorAction SilentlyContinue) { throw 'WestieLib is running; close it before running the checks' }

# Physical pixels, not coordinates virtualized for a DPI-unaware PowerShell
[void][W]::SetThreadDpiAwarenessContext([IntPtr](-4))

$KeyPath = 'HKCU:\Software\WestieLib\Window'
$StateNames = 'Left', 'Top', 'Right', 'Bottom', 'Maximized', 'PlaylistsWidthDips', 'InspectorWidthDips', 'LogHeightDips', 'LogVisible'
$script:failures = 0
$script:dpi = 96
$app = $null

function Check([string]$what, $actual, $expected) {
	$ok = "$actual" -eq "$expected"
	if (-not $ok) { $script:failures++ }
	"{0,-56} {1,-26} {2}" -f $what, $actual, $(if ($ok) { 'ok' } else { "FAIL (expected $expected)" })
}

# For checks that allow a range: $ok says whether $actual passes
function Check-That([string]$what, $actual, [bool]$ok, [string]$expected) {
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

# Closes with WM_CLOSE, or with File > Exit
function Stop-App($app, [switch]$ExitCommand) {
	if ($ExitCommand) { Send-Command $app.Hwnd 'E&xit' } else { [void][W]::PostMessageW($app.Hwnd, 0x10, [IntPtr]::Zero, [IntPtr]::Zero) }
	if (-not $app.Process.WaitForExit(5000)) { throw 'Process did not exit' }
	Check '  exit code' $app.Process.ExitCode 0
}

# Posts WM_COMMAND for the menu item with this text (without its shortcut)
function Send-Command($hwnd, [string]$item) {
	$id = [W]::CommandOf([W]::GetMenu($hwnd), $item)
	[void][W]::PostMessageW($hwnd, 0x111, [IntPtr][int]$id, [IntPtr]::Zero)
}

# The app sets menu item states in WM_INITMENUPOPUP, so send it for every popup before reading them
function Update-MenuStates($hwnd) {
	$menu = [W]::GetMenu($hwnd)
	for ($i = 0; $i -lt [W]::GetMenuItemCount($menu); $i++) {
		[void][W]::SendMessageW($hwnd, 0x117, [W]::GetSubMenu($menu, $i), [IntPtr]$i)
	}
}

function Wait-OwnedWindow($owner) {
	for ($i = 0; $i -lt 50; $i++) {
		$window = [W]::OwnedWindow($owner)
		if ($window -ne [IntPtr]::Zero) { return $window }
		Start-Sleep -Milliseconds 100
	}
	[IntPtr]::Zero
}

function Get-ClientWidth($hwnd) {
	$c = New-Object W+RECT
	[void][W]::GetClientRect($hwnd, [ref]$c)
	$c.Right
}

function Get-ClientHeight($hwnd) {
	$c = New-Object W+RECT
	[void][W]::GetClientRect($hwnd, [ref]$c)
	$c.Bottom
}

function Format-Rect($r) { "$($r.Left),$($r.Top) $($r.Right - $r.Left)x$($r.Bottom - $r.Top)" }

function Get-ToolbarStrip($hwnd) { ([W]::Children($hwnd, 'WestieLib.MainToolbar'))[0] }

function Get-StatusBar($hwnd) { ([W]::Children($hwnd, 'msctls_statusbar32'))[0] }

function Get-RowTop($hwnd) { [W]::RectIn((Get-ToolbarStrip $hwnd), $hwnd).Bottom }

# Playlists, tracks and inspector: the visible panels whose top is the row's, by their left edge
function Get-RowPanels($hwnd) {
	$top = Get-RowTop $hwnd
	@([W]::Children($hwnd, 'WestieLib.Panel') | Where-Object { [W]::IsWindowVisible($_) -and [W]::RectIn($_, $hwnd).Top -eq $top })
}

# The other panel, shown or not
function Get-LogPanel($hwnd) {
	$top = Get-RowTop $hwnd
	@([W]::Children($hwnd, 'WestieLib.Panel') | Where-Object { -not [W]::IsWindowVisible($_) -or [W]::RectIn($_, $hwnd).Top -ne $top })[0]
}

function Test-Upright($hwnd, $child) { $r = [W]::RectIn($child, $hwnd); ($r.Right - $r.Left) -lt ($r.Bottom - $r.Top) }

# The visible splitters between the panels, by their left edge
function Get-ColumnSplitters($hwnd) {
	@([W]::Children($hwnd, 'WestieLib.Splitter') | Where-Object { [W]::IsWindowVisible($_) -and (Test-Upright $hwnd $_) })
}

function Get-LogSplitter($hwnd) {
	@([W]::Children($hwnd, 'WestieLib.Splitter') | Where-Object { -not [W]::IsWindowVisible($_) -or -not (Test-Upright $hwnd $_) })[0]
}

function Get-LogVisibility($hwnd) { "$([W]::IsWindowVisible((Get-LogPanel $hwnd))) $([W]::IsWindowVisible((Get-LogSplitter $hwnd)))" }

# Bottom of the panel row: the log splitter while the log shows, else the status bar
function Get-RowBottom($hwnd) {
	$splitter = Get-LogSplitter $hwnd
	if ([W]::IsWindowVisible($splitter)) { [W]::RectIn($splitter, $hwnd).Top } else { [W]::RectIn((Get-StatusBar $hwnd), $hwnd).Top }
}

# Checks playlists | tracks | inspector widths (within $tolerance px), that the splitters fill the gaps exactly, and that
# all of them fill the height between the toolbar and the log or status bar
function Check-Widths([string]$what, $hwnd, [int[]]$expected, [int]$tolerance = 0) {
	$panels = @(Get-RowPanels $hwnd | ForEach-Object { [W]::RectIn($_, $hwnd) })
	$splitters = @(Get-ColumnSplitters $hwnd | ForEach-Object { [W]::RectIn($_, $hwnd) })
	$top = Get-RowTop $hwnd
	$bottom = Get-RowBottom $hwnd
	$offRow = @($panels + $splitters | Where-Object { $_.Top -ne $top -or $_.Bottom -ne $bottom })
	$contiguous = $panels[0].Left -eq 0 -and $panels[0].Right -eq $splitters[0].Left -and $splitters[0].Right -eq $panels[1].Left -and
		$panels[1].Right -eq $splitters[1].Left -and $splitters[1].Right -eq $panels[2].Left -and $panels[2].Right -eq (Get-ClientWidth $hwnd) -and
		$offRow.Count -eq 0
	$widths = @($panels | ForEach-Object { $_.Right - $_.Left })
	$near = $contiguous
	for ($i = 0; $i -lt 3; $i++) { if ([math]::Abs($widths[$i] - $expected[$i]) -gt $tolerance) { $near = $false } }
	$actual = "$($widths -join ' | ')$(if (-not $contiguous) { ' (gaps!)' })"
	if (-not $near) { $script:failures++ }
	"{0,-56} {1,-26} {2}" -f $what, $actual, $(if ($near) { 'ok' } else { "FAIL (expected $($expected -join ' | '))" })
}

# The toolbar strip spans the top and the status bar the bottom, each at its height
function Check-Bars([string]$what, $hwnd) {
	$cw = Get-ClientWidth $hwnd
	$ch = Get-ClientHeight $hwnd
	$strip = Format-Rect ([W]::RectIn((Get-ToolbarStrip $hwnd), $hwnd))
	$status = Format-Rect ([W]::RectIn((Get-StatusBar $hwnd), $hwnd))
	Check $what "$strip | $status" "0,0 $($cw)x$(S 34) | 0,$($ch - (S 23)) $($cw)x$(S 23)"
}

# Toolbar buttons: their height and position, the spacing, and room for their text in the font at the window's DPI
function Check-Toolbar([string]$what, $hwnd) {
	$strip = Get-ToolbarStrip $hwnd
	$toolbar = ([W]::Children($strip, 'ToolbarWindow32'))[0]
	$height = ([int64][W]::SendMessageW($toolbar, 0x43A, [IntPtr]::Zero, [IntPtr]::Zero)) -shr 16
	$r = [W]::RectIn($toolbar, $strip)
	Check "$what button height, left, top (centered)" "$height $($r.Left) $($r.Top)" "$(S 26) $(S 4) $([math]::Floor(((S 34) - 1 - (S 26)) / 2))"
	Check "$what spacing" ([W]::ToolbarSpacing($toolbar)) "gaps $(S 1) $(S 1) $(S 1) $(S 1) | separator $(S 11)"
	# Padding, icon and gap, plus a few pixels the toolbar keeps around the text itself. A button whose text was measured
	# in an older, smaller font falls short of this.
	$least = 2 * (S 8) + (S 16) + (S 5)
	$actual = [W]::ToolbarExtraWidths($toolbar, $script:dpi)
	$extras = @([regex]::Matches($actual, '\+(-?\d+)') | ForEach-Object { [int]$_.Groups[1].Value })
	$ok = $extras.Count -eq 3 -and @($extras | Select-Object -Unique).Count -eq 1 -and $extras[0] -ge $least -and $extras[0] -le $least + (S 4)
	Check-That "$what width beyond the text" $actual $ok "the same for each, $least to $($least + (S 4))"

	# Disabled Cancel's text against the same text enabled, for a moment: same place, same antialiasing
	$offset = (S 8) + (S 16) + (S 5)
	$disabled = [W]::ToolbarTextInk($hwnd, $toolbar, 2, $offset)
	$cancel = [IntPtr][int][W]::CommandOf([W]::GetMenu($hwnd), '&Cancel')
	[void][W]::SendMessageW($toolbar, 0x401, $cancel, [IntPtr]1)
	Start-Sleep -Milliseconds 200
	$enabled = [W]::ToolbarTextInk($hwnd, $toolbar, 2, $offset)
	[void][W]::SendMessageW($toolbar, 0x401, $cancel, [IntPtr]0)
	Check "$what disabled text, against enabled" $disabled $enabled
}

# Everything that scales with the DPI: bars, toolbar, status bar, and the default panel widths
function Check-Scaled($hwnd) {
	$cw = Get-ClientWidth $hwnd
	Check-Bars 'Toolbar strip | status bar' $hwnd
	Check-Toolbar ' ' $hwnd
	Check-StatusParts 'Status bar part edges' $hwnd
	Check-StatusTexts ' ' $hwnd
	Check-Widths 'Default widths' $hwnd @((S 250), ($cw - (S 250) - (S 290) - 2 * (S 5)), (S 290))
}

# Run, Library and Tools from the left, then Version running to the right edge. Version's rect stops where the size
# grip starts, or at the edge when the window is maximized and shows no grip. Separators follow every part but Version.
function Check-StatusParts([string]$what, $hwnd) {
	$status = Get-StatusBar $hwnd
	$cw = Get-ClientWidth $hwnd
	$edges = @((S 270), ((S 270) + (S 300)), ((S 270) + (S 300) + (S 220)), ($cw - (S 80)))
	$grip = $cw - [W]::StatusPartRect($status, 4).Right
	$fits = if ([W]::IsZoomed($hwnd)) { $grip -eq 0 } else { $grip -gt 0 -and $grip -le (S 20) }
	Check $what "$([W]::StatusParts($status)) | stops at grip: $fits" "$($edges -join ' ') -1 | stops at grip: True"

	$lines = @([W]::StatusSeparators($hwnd, $status))
	$matching = $lines.Count -eq $edges.Count
	for ($i = 0; $matching -and $i -lt $edges.Count; $i++) { $matching = [math]::Abs($lines[$i] - $edges[$i]) -le 2 }
	Check-That "$what, separators" ($lines -join ' ') $matching "one within 2 px of each of $($edges -join ' '), none after"
}

# Ready starts after the part's padding (glyphs may start a pixel or two later); Version is centered in its part
function Check-StatusTexts([string]$what, $hwnd) {
	$status = Get-StatusBar $hwnd
	$run = [W]::StatusTextGaps($hwnd, $status, 0)
	Check-That "$what Ready's left gap" "$($run[0])" ($run[0] -ge (S 8) -and $run[0] -le (S 8) + 2) "$(S 8) to $((S 8) + 2)"
	$version = [W]::StatusTextGaps($hwnd, $status, 4)
	Check-That "$what Version's left | right gap" "$($version[0]) | $($version[1])" ([math]::Abs($version[0] - $version[1]) -le 1 -and $version[0] -ge (S 8)) 'equal within 1, at least the padding'
}

function LParam([int]$x, [int]$y) { [IntPtr]([int64]($y -band 0xFFFF) * 65536 + ($x -band 0xFFFF)) }

function Get-Splitter($hwnd, [int]$index) { (Get-ColumnSplitters $hwnd)[$index] }

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

$userState = if (Test-Path $KeyPath) { Get-ItemProperty $KeyPath } else { $null }
Remove-Item 'HKCU:\Software\WestieLib' -Recurse -Confirm:$false -ErrorAction SilentlyContinue

try {
	"--- Run 1: no saved state"
	$app = Start-App
	$h = $app.Hwnd
	"DPI $script:dpi"
	Check 'DPI awareness (2 = per monitor)' ([W]::GetAwarenessFromDpiAwarenessContext([W]::GetWindowDpiAwarenessContext($h))) 2
	Check 'Big and small icons set' "$([W]::SendMessageW($h, 0x7F, [IntPtr]1, [IntPtr]::Zero) -ne 0) $([W]::SendMessageW($h, 0x7F, [IntPtr]0, [IntPtr]::Zero) -ne 0)" 'True True'
	$menu = [W]::GetMenu($h)
	Check 'Menu bar' ([W]::MenuText($menu)) '&File &Sync &Track &View T&ools &Help'
	$popups = @(
		'&Add playlist... [Ctrl+N], &Remove playlist..., -, &Open library folder, -, E&xit [Ctrl+Q]',
		'Sync &all [F5], Sync &selected playlist, -, &Cancel [Esc]',
		'&Confirm match, C&hoose video..., &Edit track... [Enter], -, &Retry, &Keep, &Move to Recycle Bin..., -, &Show in folder, Open on &YouTube, Open source &page',
		'&Log [Ctrl+L], -, &Columns...',
		'&Settings... [Ctrl+Alt+S], Check for &updates, -, &Rebuild library from files..., Library folder &moved..., -, Open &log folder',
		'&About WestieLib'
	)
	for ($i = 0; $i -lt $popups.Count; $i++) {
		Check "  $(([W]::MenuText($menu) -split ' ')[$i]) items" ([W]::PopupText([W]::GetSubMenu($menu, $i))) $popups[$i]
	}
	Update-MenuStates $h
	Check 'Grayed while idle with nothing selected' ([W]::ItemsWithState($menu, 0x1)) ('&Remove playlist..., Sync &selected playlist, &Cancel, &Confirm match, C&hoose video..., ' +
		'&Edit track..., &Retry, &Keep, &Move to Recycle Bin..., &Show in folder, Open on &YouTube, Open source &page')
	Check 'Checked items' ([W]::ItemsWithState($menu, 0x8)) ''
	Send-Command $h '&About WestieLib'
	$about = Wait-OwnedWindow $h
	Check 'Help > About opens a dialog' "$([W]::ClassOf($about)) $([W]::TextOf($about))" '#32770 About WestieLib'
	[void][W]::PostMessageW($about, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 300
	Check '  and closes' ([W]::IsWindow($about)) 'False'
	Check 'Initial window size' ((Get-Bounds $h) -replace '^.* ', '') "$(S 1280)x$(S 800)"
	Check-Bars 'Toolbar strip | status bar' $h
	$toolbar = ([W]::Children((Get-ToolbarStrip $h), 'ToolbarWindow32'))[0]
	Check 'Toolbar buttons' ([W]::ToolbarButtons($toolbar)) 'Add playlist, Sync all v, Cancel (disabled), -, Settings'
	Check-Toolbar ' ' $h
	Check-StatusParts 'Status bar part edges' $h
	Check-StatusTexts ' ' $h
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

	"--- Run 2: restore a normal window"
	$app = Start-App
	$h = $app.Hwnd
	$cw = Get-ClientWidth $h
	Check 'Restored bounds' (Get-Bounds $h) "100,100 $(S 1200)x$(S 750)"
	Check-Widths 'Restored widths' $h @((S 250), ($cw - (S 250) - (S 500) - 2 * $g), (S 500))
	Check-Bars '  toolbar strip | status bar' $h
	Check-Log 'Restored log' $h (S 200)
	if ($Screenshot) { Save-Screenshot $h $Screenshot }
	Send-Command $h '&Log'
	Start-Sleep -Milliseconds 200
	Check 'View > Log hides it' (Get-LogVisibility $h) 'False False'
	[void][W]::ShowWindow($h, 3)
	Start-Sleep -Milliseconds 300
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

	"--- Run 5: moved to a monitor with another DPI"
	Remove-Item 'HKCU:\Software\WestieLib' -Recurse -Confirm:$false -ErrorAction SilentlyContinue
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

		"--- Run 6: restored onto that monitor"
		$app = Start-App
		$h = $app.Hwnd
		"DPI $script:dpi"
		Check-Scaled $h
		Stop-App $app
	}
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
