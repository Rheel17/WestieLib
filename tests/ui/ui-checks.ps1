# Drives WestieLib.exe through Win32 messages and checks the main window: DPI awareness, icons, menus and their
# states, commands and shortcuts, the toolbar and status bar, the side panels' first-run content, sizes, splitter drags
# and limits, the log pane, painting during a drag, the library folder, the Settings dialog, the tools' state, adding
# playlists and looking them up, and saving/restoring the window state and settings.json across restarts. A fake
# yt-dlp and a local stand-in for Spotify and GitHub answer the look-ups, so nothing reaches the network.
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File tests/ui/ui-checks.ps1 -Exe <WestieLib.exe> [-Screenshot <png>]
#
# The exit code is the number of failed checks. The user's saved window state is set aside and put back afterwards. The
# app's files go to a scratch folder (WESTIELIB_DATA_DIR), which is deleted at the end.

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
	[DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr h);
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
	[DllImport("user32.dll")] public static extern int GetScrollPos(IntPtr h, int bar);

	// Presses Ctrl+key (or Ctrl+Alt+key) in the app without real input, so nothing can reach the user's other windows.
	// While the threads are attached they share their key state, so the app's TranslateAccelerator sees Ctrl held.
	public static void PostCtrlKey(IntPtr h, int key, bool alt = false) {
		int pid;
		uint thread = GetWindowThreadProcessId(h, out pid), self = GetCurrentThreadId();
		AttachThreadInput(self, thread, true);
		var state = new byte[256];
		GetKeyboardState(state);
		var saved = (byte[])state.Clone();
		try {
			state[0x11] |= 0x80;
			if (alt) state[0x12] |= 0x80;
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

	[DllImport("kernel32.dll")] static extern bool WriteProcessMemory(IntPtr process, IntPtr address, byte[] buffer, IntPtr size, out IntPtr written);

	// Sends a message whose lparam is a struct that points at a text buffer, both in the window's process. `build` fills
	// the struct, given the buffer's address; the text comes back from the buffer.
	static string SendForText(IntPtr h, uint msg, IntPtr w, int structSize, Func<long, byte[]> build) {
		const int TextOffset = 256, TextBytes = 2048;
		int pid;
		GetWindowThreadProcessId(h, out pid);
		IntPtr process = OpenProcess(0x38, false, pid);
		IntPtr remote = VirtualAllocEx(process, IntPtr.Zero, (IntPtr)(TextOffset + TextBytes), 0x3000, 0x04);
		try {
			var request = build((long)remote + TextOffset);
			IntPtr done;
			WriteProcessMemory(process, remote, request, (IntPtr)structSize, out done);
			SendMessageW(h, msg, w, remote);
			var text = new byte[TextBytes];
			ReadProcessMemory(process, (IntPtr)((long)remote + TextOffset), text, (IntPtr)TextBytes, out done);
			return Utf16(text);
		} finally {
			VirtualFreeEx(process, remote, IntPtr.Zero, 0x8000);
			CloseHandle(process);
		}
	}

	// The text of tool `id` of `window`, from whichever of its process's tooltip windows has it. A tooltip's owner is
	// the top-level window, not the window its tools are in. TOOLINFOW on x64: hwnd at 8, uId at 16, lpszText at 48,
	// 72 bytes.
	public static string TooltipText(IntPtr window, int id) {
		int pid, owner;
		GetWindowThreadProcessId(window, out pid);
		var tooltips = new List<IntPtr>();
		EnumWindows((h, l) => {
			GetWindowThreadProcessId(h, out owner);
			if (owner == pid && ClassOf(h) == "tooltips_class32") tooltips.Add(h);
			return true;
		}, IntPtr.Zero);
		foreach (var tooltip in tooltips) {
			var found = SendForText(tooltip, 0x438, (IntPtr)1024, 72, text => {
				var info = new byte[72];
				BitConverter.GetBytes(72).CopyTo(info, 0);
				BitConverter.GetBytes((long)window).CopyTo(info, 8);
				BitConverter.GetBytes((long)id).CopyTo(info, 16);
				BitConverter.GetBytes(text).CopyTo(info, 48);
				return info;
			});
			if (found.Length > 0) return found;
		}
		return "";
	}

	// A list view row's cells in display order, joined with " | ". LVITEMW on x64: iSubItem at 8, pszText at 24,
	// cchTextMax at 32, 88 bytes.
	public static string ListViewRow(IntPtr list, int item) {
		var header = SendMessageW(list, 0x101F, IntPtr.Zero, IntPtr.Zero);
		int count = (int)SendMessageW(header, 0x1200, IntPtr.Zero, IntPtr.Zero);
		long result;
		var order = SendForBuffer(list, 0x103B, (IntPtr)count, count * 4, out result);
		var cells = new List<string>();
		for (int i = 0; i < count; i++) {
			int column = BitConverter.ToInt32(order, i * 4);
			cells.Add(SendForText(list, 0x1073, (IntPtr)item, 88, text => {
				var row = new byte[88];
				BitConverter.GetBytes(column).CopyTo(row, 8);
				BitConverter.GetBytes(text).CopyTo(row, 24);
				BitConverter.GetBytes(1024).CopyTo(row, 32);
				return row;
			}));
		}
		return String.Join(" | ", cells);
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

	// The pixels of a child's first columns in a PrintWindow capture, as a string to compare
	public static string ChildPixels(IntPtr window, IntPtr child, int width) {
		RECT area;
		GetClientRect(child, out area);
		int dx, dy;
		var pixels = new StringBuilder();
		using (var bitmap = Capture(window, child, out dx, out dy)) {
			for (int x = 0; x < Math.Min(width, area.Right); x++) {
				for (int y = 0; y < area.Bottom; y++) pixels.Append(bitmap.GetPixel(x + dx, y + dy).ToArgb().ToString("x8"));
			}
		}
		return pixels.ToString();
	}

	// The colors at points (x, y pairs) of a child's client area in a PrintWindow capture, as RRGGBB
	public static string[] PixelsIn(IntPtr window, IntPtr child, int[] points) {
		int dx, dy;
		var colors = new List<string>();
		using (var bitmap = Capture(window, child, out dx, out dy)) {
			for (int i = 0; i + 1 < points.Length; i += 2) {
				var p = bitmap.GetPixel(points[i] + dx, points[i + 1] + dy);
				colors.Add(String.Format("{0:x2}{1:x2}{2:x2}", p.R, p.G, p.B));
			}
		}
		return colors.ToArray();
	}

	[DllImport("user32.dll")] static extern bool SystemParametersInfoW(uint action, uint param, out bool value, uint winIni);

	// Whether Windows animates controls (SPI_GETCLIENTAREAANIMATION)
	public static bool ClientAreaAnimation() {
		bool on;
		SystemParametersInfoW(0x1042, 0, out on, 0);
		return on;
	}

	// The color of the icon in a status bar part: green (ready), blue (working), amber (attention), or none
	public static string StatusIconColor(IntPtr window, IntPtr bar, int part) {
		RECT area = StatusPartRect(bar, part);
		int dx, dy, green = 0, blue = 0, amber = 0;
		using (var bitmap = Capture(window, bar, out dx, out dy)) {
			for (int x = area.Left; x < area.Right; x++) {
				for (int y = area.Top; y < area.Bottom; y++) {
					var p = bitmap.GetPixel(x + dx, y + dy);
					if (p.G > p.R + 40 && p.G > p.B + 40) green++;
					else if (p.B > p.R + 60 && p.B > p.G + 30) blue++;
					else if (p.R > 200 && p.G > 120 && p.G < 220 && p.B < 90) amber++;
				}
			}
		}
		int most = Math.Max(green, Math.Max(blue, amber));
		if (most < 8) return "none";
		return most == green ? "green" : most == blue ? "blue" : "amber";
	}

	[StructLayout(LayoutKind.Sequential)] public struct MENUBARINFO { public int Size; public RECT Bar; public IntPtr Menu, MenuWindow; public int Flags; }
	[DllImport("user32.dll")] public static extern bool GetMenuBarInfo(IntPtr h, int idObject, int idItem, ref MENUBARINFO info);

	// The colors in the menu bar's last row and the rows below it down to the client area, most common first
	public static string MenuBarEdgeColors(IntPtr window) {
		var bar = new MENUBARINFO();
		bar.Size = Marshal.SizeOf(bar);
		GetMenuBarInfo(window, unchecked((int)0xFFFFFFFD), 0, ref bar);
		RECT frame;
		GetWindowRect(window, out frame);
		var counts = new Dictionary<string, int>();
		int dx, dy;
		using (var bitmap = Capture(window, window, out dx, out dy)) {
			for (int y = bar.Bar.Bottom - 1 - frame.Top; y < dy; y++) {
				for (int x = bar.Bar.Left - frame.Left; x < bar.Bar.Right - frame.Left; x++) {
					var p = bitmap.GetPixel(x, y);
					var key = String.Format("#{0:X2}{1:X2}{2:X2}", p.R, p.G, p.B);
					counts[key] = counts.ContainsKey(key) ? counts[key] + 1 : 1;
				}
			}
		}
		return String.Join(" ", counts.OrderByDescending(c => c.Value).Select(c => c.Key));
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
		// Long enough for an error message with two paths in it
		var s = new StringBuilder(4096);
		GetWindowTextW(h, s, s.Capacity);
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

	[DllImport("user32.dll", CharSet = CharSet.Unicode, EntryPoint = "SendMessageW")] static extern IntPtr SendText(IntPtr h, uint msg, IntPtr w, StringBuilder l);
	[DllImport("user32.dll", CharSet = CharSet.Unicode, EntryPoint = "SendMessageW")] static extern IntPtr SendString(IntPtr h, uint msg, IntPtr w, string l);
	[DllImport("user32.dll")] public static extern int GetWindowLongW(IntPtr h, int index);

	// An edit keeps its text itself, so GetWindowTextW from another process doesn't see it; WM_GETTEXT and WM_SETTEXT
	// are marshalled
	public static string EditText(IntPtr edit) {
		var s = new StringBuilder(1024);
		SendText(edit, 0x0D, (IntPtr)1024, s);
		return s.ToString();
	}

	public static void SetEditText(IntPtr edit, string text) { SendString(edit, 0x0C, IntPtr.Zero, text); }

	[DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr dialog, int id);

	[DllImport("user32.dll")] static extern IntPtr SetFocus(IntPtr h);

	// Gives a window the keyboard focus, as a click into it would: attached, this thread shares the app's input state
	public static void Focus(IntPtr h) {
		int pid;
		uint thread = GetWindowThreadProcessId(h, out pid), self = GetCurrentThreadId();
		AttachThreadInput(self, thread, true);
		SetFocus(h);
		AttachThreadInput(self, thread, false);
	}

	[StructLayout(LayoutKind.Sequential)] public struct GUITHREADINFO { public int Size; public uint Flags; public IntPtr Active, Focus, Capture, MenuOwner, MoveSize, Caret; public RECT CaretRect; }
	[DllImport("user32.dll")] static extern bool GetGUIThreadInfo(uint thread, ref GUITHREADINFO info);

	// The window that has the keyboard focus on h's thread
	public static IntPtr FocusOf(IntPtr h) {
		int pid;
		var info = new GUITHREADINFO();
		info.Size = Marshal.SizeOf(info);
		GetGUIThreadInfo(GetWindowThreadProcessId(h, out pid), ref info);
		return info.Focus;
	}

	// Whether a child's text is drawn in red (the error color) in a PrintWindow capture
	// A visible top-level window of a process, such as a message box shown before any other window
	public static IntPtr WindowOfProcess(int pid, string cls) {
		IntPtr found = IntPtr.Zero;
		EnumWindows((h, l) => {
			int owner;
			GetWindowThreadProcessId(h, out owner);
			if (owner == pid && IsWindowVisible(h) && ClassOf(h) == cls) { found = h; return false; }
			return true;
		}, IntPtr.Zero);
		return found;
	}

	public static bool RedInk(IntPtr window, IntPtr child) {
		RECT client;
		GetClientRect(child, out client);
		int dx, dy, red = 0;
		using (var bitmap = Capture(window, child, out dx, out dy)) {
			for (int x = 0; x < client.Right; x++) {
				for (int y = 0; y < client.Bottom; y++) {
					var p = bitmap.GetPixel(x + dx, y + dy);
					if (p.R > p.G + 60 && p.R > p.B + 60) red++;
				}
			}
		}
		return red > 10;
	}

	[StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
	[DllImport("user32.dll")] public static extern IntPtr WindowFromPoint(POINT p);
	[DllImport("user32.dll")] public static extern IntPtr GetAncestor(IntPtr h, uint flags);

	static IntPtr Pack(int x, int y) { return (IntPtr)(((y & 0xFFFF) << 16) | (x & 0xFFFF)); }

	// Drags a splitter `by` pixels and back, one pixel per step, as a mouse would. False when the splitter didn't get
	// there: it stops following once it loses the mouse capture, say to a click in another window.
	public static bool DragSplitter(IntPtr window, IntPtr splitter, int by) {
		int from = RectIn(splitter, window).Left, step = by > 0 ? 1 : -1;
		SendMessageW(splitter, 0x201, (IntPtr)1, Pack(2, 10));
		for (int d = step; d != by + step; d += step) SendMessageW(splitter, 0x200, (IntPtr)1, Pack(from + d - RectIn(splitter, window).Left + 2, 10));
		bool followed = RectIn(splitter, window).Left == from + by;
		for (int d = by - step; d != -step; d -= step) SendMessageW(splitter, 0x200, (IntPtr)1, Pack(from + d - RectIn(splitter, window).Left + 2, 10));
		SendMessageW(splitter, 0x202, IntPtr.Zero, Pack(2, 10));
		return followed;
	}

	// The screen's pixels in a rect, as DWM composed them. Unlike PrintWindow, which paints the window afresh, this sees
	// a window between an erase and a paint.
	static byte[] ScreenPixels(RECT r) {
		using (var bitmap = new System.Drawing.Bitmap(r.Right - r.Left, r.Bottom - r.Top))
		using (var graphics = System.Drawing.Graphics.FromImage(bitmap)) {
			graphics.CopyFromScreen(r.Left, r.Top, 0, 0, bitmap.Size);
			var data = bitmap.LockBits(new System.Drawing.Rectangle(0, 0, bitmap.Width, bitmap.Height), System.Drawing.Imaging.ImageLockMode.ReadOnly,
				System.Drawing.Imaging.PixelFormat.Format32bppArgb);
			var bytes = new byte[data.Stride * bitmap.Height];
			Marshal.Copy(data.Scan0, bytes, 0, bytes.Length);
			bitmap.UnlockBits(data);
			return bytes;
		}
	}

	static int DarkPixels(byte[] p) {
		int n = 0;
		for (int i = 0; i < p.Length; i += 4) if ((p[i] + p[i + 1] + p[i + 2]) / 3 < 140) n++;
		return n;
	}

	static bool Differ(byte[] a, byte[] b) {
		for (int i = 0; i < a.Length; i++) if (Math.Abs(a[i] - b[i]) > 24) return true;
		return false;
	}

	static volatile bool watching;

	// Captures `area` (client coordinates of `window`) over and over while a splitter drag runs, and counts the captures
	// that blinked: for a steady area, any that differ from the capture before the drag; for moving text, any that lost
	// a tenth of its dark pixels. "covered" when another window is over the area, since then the screen shows that one.
	public static string WatchDrag(IntPtr window, RECT area, IntPtr splitter, int by, bool moving) {
		MapWindowPoints(window, IntPtr.Zero, ref area, 2);
		foreach (var p in new[] { new POINT { X = area.Left, Y = area.Top }, new POINT { X = area.Right - 1, Y = area.Bottom - 1 } }) {
			if (GetAncestor(WindowFromPoint(p), 2) != window) return "covered";
		}
		var before = ScreenPixels(area);
		int ink = DarkPixels(before), frames = 0, blinked = 0;
		watching = true;
		var thread = new System.Threading.Thread(() => {
			SetThreadDpiAwarenessContext((IntPtr)(-4));
			while (watching) {
				var now = ScreenPixels(area);
				frames++;
				if (moving ? DarkPixels(now) * 10 < ink * 9 : Differ(now, before)) blinked++;
			}
		});
		thread.Start();
		bool followed = DragSplitter(window, splitter, by);
		watching = false;
		thread.Join();
		if (!followed) return "the splitter didn't follow the drag";
		return frames < 20 ? "only " + frames + " captures" : "blinked " + blinked;
	}
}
'@

# A local stand-in for open.spotify.com, and for GitHub's API when it has a yt-dlp to serve, so the checks never reach
# the network. Embed pages for the ids in SpotifyNames hold that name; other ids get Spotify's error page.
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System.Threading;

public class FakeWeb {
	readonly TcpListener listener = new TcpListener(IPAddress.Loopback, 0);
	public readonly Dictionary<string, string> SpotifyNames = new Dictionary<string, string>();
	public readonly HashSet<string> SlowIds = new HashSet<string>();
	public readonly HashSet<string> BrokenIds = new HashSet<string>();
	// The yt-dlp download waits for it, so a check can look at the app while yt-dlp installs
	public readonly ManualResetEvent AssetGate = new ManualResetEvent(false);
	public byte[] YtDlp;
	public int Port;
	public string Url { get { return "http://127.0.0.1:" + Port; } }

	public FakeWeb() {
		listener.Start();
		Port = ((IPEndPoint)listener.LocalEndpoint).Port;
		var thread = new Thread(Accept);
		thread.IsBackground = true;
		thread.Start();
	}

	public void Stop() {
		AssetGate.Set();
		listener.Stop();
	}

	void Accept() {
		while (true) {
			TcpClient client;
			try { client = listener.AcceptTcpClient(); } catch { return; }
			ThreadPool.QueueUserWorkItem(_ => Serve(client));
		}
	}

	void Serve(TcpClient client) {
		try {
			using (client)
			using (var stream = client.GetStream()) {
				var reader = new StreamReader(stream, Encoding.ASCII);
				string path = reader.ReadLine().Split(' ')[1];
				while (!string.IsNullOrEmpty(reader.ReadLine())) { }
				int status = 200;
				byte[] body = Answer(path, ref status);
				byte[] head = Encoding.ASCII.GetBytes("HTTP/1.1 " + status + " Fake\r\nContent-Length: " + body.Length + "\r\nConnection: close\r\n\r\n");
				stream.Write(head, 0, head.Length);
				stream.Write(body, 0, body.Length);
			}
		} catch { }
	}

	byte[] Answer(string path, ref int status) {
		const string Embed = "/embed/playlist/";
		if (path.StartsWith(Embed)) {
			string id = path.Substring(Embed.Length);
			if (SlowIds.Contains(id)) Thread.Sleep(1500);
			if (BrokenIds.Contains(id)) { status = 500; return new byte[0]; }
			string name;
			string data = SpotifyNames.TryGetValue(id, out name)
				? "{\"props\":{\"pageProps\":{\"state\":{\"data\":{\"entity\":{\"name\":\"" + name + "\",\"type\":\"playlist\"}}}}}}"
				: "{\"props\":{\"pageProps\":{\"status\":500,\"title\":\"Page not available\"}}}";
			return Encoding.UTF8.GetBytes("<html><body><script id=\"__NEXT_DATA__\" type=\"application/json\">" + data + "</script></body></html>");
		}
		if (path == "/repos/yt-dlp/yt-dlp/releases/latest" && YtDlp != null) {
			string digest;
			using (var sha = SHA256.Create()) digest = BitConverter.ToString(sha.ComputeHash(YtDlp)).Replace("-", "").ToLowerInvariant();
			return Encoding.UTF8.GetBytes("{\"tag_name\":\"2026.10.22\",\"assets\":[{\"name\":\"yt-dlp.exe\",\"browser_download_url\":\"" + Url +
				"/download/yt-dlp.exe\",\"size\":" + YtDlp.Length + ",\"digest\":\"sha256:" + digest + "\"}]}");
		}
		if (path == "/download/yt-dlp.exe" && YtDlp != null) {
			AssetGate.WaitOne();
			return YtDlp;
		}
		status = 404;
		return new byte[0];
	}
}
'@

# A yt-dlp that answers a playlist look-up by the list id, as the real one does: a name, YouTube's errors for ids
# starting with PLmissing and PLprivate, a network error for PLbroken, and a name 1.5 s late for PLslow. Each run adds
# its arguments, joined with |, to yt-dlp-runs.txt beside it.
$FakeYtDlpSource = @'
using System;
using System.IO;
using System.Text;
using System.Threading;

public static class FakeYtDlp {
	public static int Main(string[] args) {
		File.AppendAllText(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "yt-dlp-runs.txt"), string.Join("|", args) + "\n", new UTF8Encoding(false));
		string url = args[args.Length - 1];
		int list = url.IndexOf("list=");
		string id = list < 0 ? "" : url.Substring(list + 5);
		if (id.StartsWith("PLslow")) Thread.Sleep(1500);
		string error = null;
		if (id.StartsWith("PLmissing")) error = "YouTube said: The playlist does not exist.";
		if (id.StartsWith("PLprivate")) error = "This playlist is private";
		if (id.StartsWith("PLbroken")) error = "Unable to download API page: HTTP Error 503: Service Unavailable";
		if (error != null) {
			Write(Console.OpenStandardError(), "WARNING: [youtube:tab] YouTube Music is not directly supported\nERROR: [youtube:tab] " + id + ": " + error + "\n");
			return 1;
		}
		// The real one escapes everything outside ASCII
		string title = id == "PLx9k2_-Ab" ? "Caf\\u00e9 Check Mix" : "Checked " + id;
		Write(Console.OpenStandardOutput(), "{\"_type\": \"playlist\", \"id\": \"" + id + "\", \"title\": \"" + title + "\", \"entries\": []}\n");
		return 0;
	}

	static void Write(Stream stream, string text) {
		byte[] bytes = Encoding.UTF8.GetBytes(text);
		stream.Write(bytes, 0, bytes.Length);
		stream.Flush();
	}
}
'@

if (Get-Process WestieLib -ErrorAction SilentlyContinue) { throw 'WestieLib is running; close it before running the checks' }

# Physical pixels, not coordinates virtualized for a DPI-unaware PowerShell
[void][W]::SetThreadDpiAwarenessContext([IntPtr](-4))

$KeyPath = 'HKCU:\Software\WestieLib\Window'
$StateNames = 'Left', 'Top', 'Right', 'Bottom', 'Maximized', 'PlaylistsWidthDips', 'InspectorWidthDips', 'LogHeightDips', 'LogVisible'
# The app keeps its files here instead of %LOCALAPPDATA%\WestieLib, so the user's settings and tools stay untouched
$DataDir = Join-Path ([IO.Path]::GetTempPath()) "WestieLib-checks-$([guid]::NewGuid().ToString('N').Substring(0, 8))"
$env:WESTIELIB_DATA_DIR = $DataDir
# Nothing listens there, so every release look-up fails at once, and never leaves the PC
$env:WESTIELIB_GITHUB_API = 'http://127.0.0.1:9'
$LookupFailure = 'Couldn''t reach 127\.0\.0\.1: .+'
$SettingsFile = Join-Path $DataDir 'settings.json'
$ToolsDir = Join-Path $DataDir 'tools'
$FakeYtDlp = Join-Path $DataDir 'fakes\yt-dlp.exe'
New-Item -ItemType Directory -Force (Split-Path $FakeYtDlp) | Out-Null
Add-Type -TypeDefinition $FakeYtDlpSource -OutputAssembly $FakeYtDlp -OutputType ConsoleApplication
$FakeYtDlpRuns = Join-Path $ToolsDir 'yt-dlp-runs.txt'
$Web = New-Object FakeWeb
$env:WESTIELIB_SPOTIFY_URL = $Web.Url
$Ellipsis = [char]0x2026
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

# Waits for the app's first tool run to end, unless told not to
function Start-App([switch]$DuringToolRun) {
	$process = Start-Process -FilePath $Exe -PassThru
	for ($i = 0; $i -lt 50; $i++) {
		Start-Sleep -Milliseconds 100
		$hwnd = [W]::FindWindowW('WestieLib.MainWindow', [NullString]::Value)
		if ($hwnd -ne [IntPtr]::Zero) { break }
	}
	if ($hwnd -eq [IntPtr]::Zero) { throw 'Main window not found' }
	Start-Sleep -Milliseconds 400
	$script:dpi = [W]::GetDpiForWindow($hwnd)
	if (-not $DuringToolRun) { Wait-ToolsIdle $hwnd }
	return @{ Process = $process; Hwnd = $hwnd }
}

# A tool run grays Check for updates until it ends. Against the checks' GitHub a look-up fails within a few seconds.
function Wait-ToolsIdle($hwnd) {
	$menu = [W]::GetMenu($hwnd)
	$id = [W]::CommandOf($menu, 'Check for &updates')
	for ($i = 0; $i -lt 150; $i++) {
		Update-MenuStates $hwnd
		if (([W]::GetMenuState($menu, $id, 0) -band 0x1) -eq 0) { return }
		Start-Sleep -Milliseconds 100
	}
	throw 'The tool run did not end'
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

# The main window's own panels; EnumChildWindows also returns the panels inside them
function Get-MainPanels($hwnd) { @([W]::Children($hwnd, 'WestieLib.Panel') | Where-Object { [W]::GetAncestor($_, 1) -eq $hwnd }) }

# The playlists, the main column (tracks or welcome) and the inspector: the visible panels whose top is the row's, by
# their left edge
function Get-RowPanels($hwnd) {
	$top = Get-RowTop $hwnd
	@(Get-MainPanels $hwnd | Where-Object { [W]::IsWindowVisible($_) -and [W]::RectIn($_, $hwnd).Top -eq $top })
}

# A panel of the main window by its name (its window text), shown or not
function Get-Panel($hwnd, [string]$name) { @(Get-MainPanels $hwnd | Where-Object { [W]::TextOf($_) -eq $name })[0] }

function Get-LogPanel($hwnd) { Get-Panel $hwnd 'Log' }

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

# The menu bar runs into the toolbar: no gray from its last row down to the client area. Windows redraws the bar's
# items as the keyboard moves through the menus, and in another color for an inactive window, so with -Menus this
# checks after that too. Posted keys reach the app's menu loop only, never another window.
function Check-MenuBarEdge([string]$what, $hwnd, [switch]$Menus) {
	Check $what ([W]::MenuBarEdgeColors($hwnd)) '#FFFFFF'
	if (-not $Menus) { return }
	$keys = @(@(0x112, 0xF100, 0), @(0x100, 0x27, 0), @(0x100, 0x1B, 0), @(0x112, 0xF100, 0x76), @(0x100, 0x1B, 0), @(0x100, 0x1B, 0))
	foreach ($key in $keys) {
		[void][W]::PostMessageW($hwnd, $key[0], [IntPtr]$key[1], [IntPtr]$key[2])
		Start-Sleep -Milliseconds 250
	}
	Check '  after F10, Right, Esc, Alt+V, Esc, Esc' ([W]::MenuBarEdgeColors($hwnd)) '#FFFFFF'
	[void][W]::SendMessageW($hwnd, 0x86, [IntPtr]::Zero, [IntPtr]::Zero)
	Check '  while the window looks inactive' ([W]::MenuBarEdgeColors($hwnd)) '#FFFFFF'
	[void][W]::SendMessageW($hwnd, 0x86, [IntPtr]1, [IntPtr]::Zero)
}

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

# Status bar parts from the left; 3 is the gap
$ToolsPart = 4
$VersionPart = 5

# A part's share of the bar's width, or its minimum in DIPs
function Get-PartWidth([int]$barWidth, [int]$minimum, [int]$percent) { [math]::Max((S $minimum), [math]::Floor($barWidth * $percent / 100)) }

# The Tools part is as wide as the widest thing it can say: the word, the icon, and what the tools are doing, with the
# padding and the gap the bar keeps between a part's edge and its rect
$ToolsWorkTexts = 'checking', 'checking for updates', 'installing yt-dlp 100%', 'updating yt-dlp 100%', 'installing ffmpeg 100%', 'updating ffmpeg 100%'
function Get-ToolsWidth($status) {
	$result = 0L
	$borders = [W]::SendForBuffer($status, 0x407, [IntPtr]::Zero, 12, [ref]$result)
	$word = [W]::TextWidth('Tools', $script:dpi) + (S 7) + (S 12)
	$widest = ($ToolsWorkTexts | ForEach-Object { $word + (S 7) + [W]::TextWidth($_, $script:dpi) } | Measure-Object -Maximum).Maximum
	[BitConverter]::ToInt32($borders, 8) + 2 * (S 8) + $widest
}

# Run, Tracks and Library from the left, each at its share of the bar or its minimum, then the gap, Tools, and Version
# running to the right edge; Library gives way when they don't fit. Version's rect stops where the size grip starts, or
# at the edge when the window is maximized and shows no grip. Separators follow every part but Version.
function Check-StatusParts([string]$what, $hwnd) {
	$status = Get-StatusBar $hwnd
	$cw = Get-ClientWidth $hwnd
	$left = @((Get-PartWidth $cw 250 22), (Get-PartWidth $cw 100 10), (Get-PartWidth $cw 150 36))
	$tools = Get-ToolsWidth $status
	$missing = ($left | Measure-Object -Sum).Sum - ($cw - $tools - (S 80))
	if ($missing -gt 0) { $left[2] = [math]::Max(0, $left[2] - $missing) }
	$edges = @()
	$right = 0
	foreach ($width in $left) { $right += $width; $edges += $right }
	$edges += [math]::Max($right, $cw - $tools - (S 80))
	$edges += $edges[-1] + $tools
	$parts = $edges
	$grip = $cw - [W]::StatusPartRect($status, $VersionPart).Right
	$fits = if ([W]::IsZoomed($hwnd)) { $grip -eq 0 } else { $grip -gt 0 -and $grip -le (S 20) }
	Check $what "$([W]::StatusParts($status)) | stops at grip: $fits" "$($parts -join ' ') -1 | stops at grip: True"

	$lines = @([W]::StatusSeparators($hwnd, $status))
	$matching = $lines.Count -eq $edges.Count
	for ($i = 0; $matching -and $i -lt $edges.Count; $i++) { $matching = [math]::Abs($lines[$i] - $edges[$i]) -le 2 }
	Check-That "$what, separators" ($lines -join ' ') $matching "one within 2 px of each of $($edges -join ' '), none after"
}

# Ready, the track count and the library folder start after their part's padding (glyphs may start a pixel or two
# later); Version is centered in its part
function Check-StatusTexts([string]$what, $hwnd) {
	$status = Get-StatusBar $hwnd
	$gaps = @(0, 1, 2 | ForEach-Object { [W]::StatusTextGaps($hwnd, $status, $_)[0] })
	$padded = @($gaps | Where-Object { $_ -ge (S 8) -and $_ -le (S 8) + 2 }).Count -eq 3
	Check-That "$what Ready's | tracks' | folder's left gap" ($gaps -join ' | ') $padded "$(S 8) to $((S 8) + 2) each"
	$version = [W]::StatusTextGaps($hwnd, $status, $VersionPart)
	Check-That "$what Version's left | right gap" "$($version[0]) | $($version[1])" ([math]::Abs($version[0] - $version[1]) -le 1 -and $version[0] -ge (S 8)) 'equal within 1, at least the padding'
}

# The left panel from the top: the All tracks row, the rule, "No playlists yet." and the Add playlist button, which is
# as wide as its padding, icon, gap and text in the font at the window's DPI
function Check-PlaylistPanel([string]$what, $hwnd) {
	$panel = (Get-RowPanels $hwnd)[0]
	$pw = Get-ClientWidth $panel
	$row = ([W]::Children($panel, 'WestieLib.AllTracksRow'))[0]
	$rowHeight = 2 + (S 5) + (S 18) + (S 16) + (S 6)
	Check "$what All tracks row" "$([W]::TextOf($row)) $(Format-Rect ([W]::RectIn($row, $panel)))" "All tracks $(S 3),$(S 4) $($pw - 2 * (S 3))x$rowHeight"
	Check "$what  selected" (Get-Selection $hwnd 0) 'All tracks'
	# Where the row's name starts: its inset, border, padding, glyph and gap
	$text = (S 3) + 1 + (S 7) + (S 16) + (S 6)
	$empty = ([W]::Children($panel, 'WestieLib.Label'))[0]
	$emptyTop = (S 4) + $rowHeight + (S 5) + 1 + (S 5) + (S 6)
	Check "$what  empty list" "$([W]::TextOf($empty)) $(Format-Rect ([W]::RectIn($empty, $panel)))" "No playlists yet. $text,$emptyTop $($pw - $text - (S 12))x$(S 16)"
	$button = ([W]::Children($panel, 'Button'))[0]
	$width = 2 * (S 10) + (S 16) + (S 5) + [W]::TextWidth('Add playlist', $script:dpi)
	$buttonTop = $emptyTop + (S 16) + (S 2) + (S 8)
	Check "$what  button" "$([W]::TextOf($button)) $(Format-Rect ([W]::RectIn($button, $panel)))" "Add playlist $text,$buttonTop $($width)x$(S 23)"
}

$WelcomeTexts = 'Welcome to WestieLib / WestieLib keeps your YouTube Music and Spotify playlists in sync with a folder of tagged audio files. ' +
	'Choose where the files go, then add your first playlist. / Library folder'
$DefaultLibrary = Join-Path ([Environment]::GetFolderPath('MyMusic')) 'WestieLib'

function Get-DirectChildren($parent, [string]$class) { @([W]::Children($parent, $class) | Where-Object { [W]::GetAncestor($_, 1) -eq $parent }) }

function Get-FolderEdit($hwnd) { ([W]::Children((Get-DirectChildren (Get-Panel $hwnd 'Welcome') 'WestieLib.Panel')[0], 'Edit'))[0] }

# The welcome screen takes the main column while the track panel hides. Its column is 430 wide (less when the panel is
# narrow) and centered both ways; from the top: icon, heading, text, label, the field (edit and Browse across the
# column, the hint line under them) and the default button.
function Check-Welcome([string]$what, $hwnd) {
	$panel = Get-Panel $hwnd 'Welcome'
	Check "$what in the main column, tracks hidden" "$((Get-RowPanels $hwnd)[1] -eq $panel) $([W]::IsWindowVisible((Get-Panel $hwnd 'Tracks')))" 'True False'
	$labels = @(Get-DirectChildren $panel 'WestieLib.Label' | Sort-Object { [W]::RectIn($_, $panel).Top })
	Check "$what  texts" (($labels | ForEach-Object { [W]::TextOf($_) }) -join ' / ') $WelcomeTexts

	$pw = Get-ClientWidth $panel
	$width = [math]::Min((S 430), $pw - 2 * (S 16))
	$heading = [W]::RectIn($labels[0], $panel)
	$label = [W]::RectIn($labels[2], $panel)
	$field = (Get-DirectChildren $panel 'WestieLib.Panel')[0]
	$fieldRect = [W]::RectIn($field, $panel)
	$button = (Get-DirectChildren $panel 'Button')[0]
	$buttonRect = [W]::RectIn($button, $panel)
	$top = $heading.Top - (S 12) - (S 40)
	# Twice the offset from the panel's center, so it stays whole
	$offset = ($top + $buttonRect.Bottom) - (Get-ClientHeight $panel)
	$actual = "$($heading.Left) $($heading.Right - $heading.Left) | $offset"
	Check-That "$what  column left, width | center offset x2" $actual ($actual -match "^$([math]::Floor(($pw - $width) / 2)) $width \| -?[012]$") "$([math]::Floor(($pw - $width) / 2)) $width | -2 to 2"

	$edit = ([W]::Children($field, 'Edit'))[0]
	$browse = ([W]::Children($field, 'Button'))[0]
	$editRect = [W]::RectIn($edit, $field)
	$browseRect = [W]::RectIn($browse, $field)
	$actual = "$(Format-Rect $editRect) | $([W]::TextOf($browse)) $($browseRect.Left - $editRect.Right) $($fieldRect.Right - $fieldRect.Left - $browseRect.Right)"
	$expected = "0,0 $($browseRect.Left - (S 8))x$(S 23) | Browse$([char]0x2026) $(S 8) 0"
	Check "$what  edit | Browse, gap, right" $actual $expected
	# The hint line keeps room for its longest hint, a line or more
	$hint = [W]::RectIn(([W]::Children($field, 'WestieLib.Label'))[0], $field)
	$hintHeight = $hint.Bottom - $hint.Top
	$actual = "$(Format-Rect $fieldRect) | $(Format-Rect $hint)"
	$expected = "$($heading.Left),$($label.Bottom + (S 4)) $($width)x$((S 23) + (S 4) + $hintHeight) | 0,$((S 23) + (S 4)) $($width)x$hintHeight"
	Check-That "$what  field | its hint line" $actual ($actual -eq $expected -and $hintHeight -ge (S 16)) "$expected, the hint at least $(S 16) tall"
	$style = [W]::GetWindowLongW($button, -16) -band 0xF
	Check "$what  button, default" "$([W]::TextOf($button)) $($buttonRect.Left) $($buttonRect.Top) $($style -eq 1)" "Add your first playlist $($heading.Left) $($fieldRect.Bottom + (S 16)) True"
}

$HintNotFullPath = "Enter a full folder path, such as $DefaultLibrary."
$HintNoSuchDrive = "That drive isn't on this PC."
$HintNotAFolder = "That's a file, not a folder."
$HintWillBeCreated = "This folder doesn't exist yet. WestieLib creates it when it first syncs."
# Folders no test leaves behind: they're only typed, never created
$Unique = [guid]::NewGuid().ToString('N').Substring(0, 8)
$MissingFolder = "C:\WestieLib-check-$Unique"
$ClosedFolder = "C:\WestieLib-$([char]0x00FC)-closed-$Unique"

function Get-FolderHint($hwnd) { ([W]::Children([W]::GetAncestor((Get-FolderEdit $hwnd), 1), 'WestieLib.Label'))[0] }

# What the hint line says right after typing $text, before the field loses focus; whether it's red; and whether the
# line moved
function Get-HintFor($hwnd, [string]$text) {
	$hint = Get-FolderHint $hwnd
	$before = Format-Rect ([W]::RectIn($hint, $hwnd))
	[W]::SetEditText((Get-FolderEdit $hwnd), $text)
	Start-Sleep -Milliseconds 100
	"$([W]::TextOf($hint)) | red $([W]::RedInk($hwnd, $hint)) | moved $((Format-Rect ([W]::RectIn($hint, $hwnd))) -ne $before)"
}

# The field reads $field, and the status bar's library text is as wide as $folder in the message font (within the
# glyphs' overhang)
function Check-FolderInEffect([string]$what, $hwnd, [string]$field, [string]$folder) {
	$status = Get-StatusBar $hwnd
	$part = [W]::StatusPartRect($status, 2)
	$gaps = [W]::StatusTextGaps($hwnd, $status, 2)
	$width = $part.Right - $part.Left - $gaps[0] - $gaps[1]
	$expected = [W]::TextWidth($folder, $script:dpi)
	$text = [W]::EditText((Get-FolderEdit $hwnd))
	Check-That $what "$text | $width" ($text -eq $field -and [math]::Abs($width - $expected) -le 3) "$field | $expected within 3"
}

function Wait-Closed($window) {
	for ($i = 0; $i -lt 30 -and [W]::IsWindow($window); $i++) { Start-Sleep -Milliseconds 100 }
	Start-Sleep -Milliseconds 300
}

# Picks $folder in an open folder picker: its only visible edit is its Folder box, and its button for picking is IDOK.
# Returns how many Folder boxes it found; with any other count than 1 it cancels the picker instead.
function Select-InPicker($picker, [string]$folder) {
	Start-Sleep -Milliseconds 500
	$boxes = @([W]::Children($picker, 'Edit') | Where-Object { [W]::IsWindowVisible($_) })
	if ($boxes.Count -eq 1) {
		[W]::SetEditText($boxes[0], $folder)
		[void][W]::PostMessageW([W]::GetDlgItem($picker, 1), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	} else {
		[void][W]::PostMessageW($picker, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
	}
	Wait-Closed $picker
	$boxes.Count
}

# The field starts at the default folder. The hint line follows the typing. Leaving the field takes a folder into
# effect, unless its hint is red. Browse works whatever the field says, and picking a folder takes effect at once.
function Check-LibraryFolder($hwnd) {
	$edit = Get-FolderEdit $hwnd
	$browse = ([W]::Children([W]::GetAncestor($edit, 1), 'Button'))[0]
	$music = [Environment]::GetFolderPath('MyMusic')
	Check-FolderInEffect 'Library folder: field | status bar text width' $hwnd $DefaultLibrary $DefaultLibrary
	$expected = if (Test-Path $DefaultLibrary -PathType Container) { '' } else { $HintWillBeCreated }
	Check '  hint for the default folder' ([W]::TextOf((Get-FolderHint $hwnd))) $expected

	$drive = @([char[]]'QRSTUVWXYZ' | Where-Object { -not (Test-Path "$($_):\") })[0]
	Check '  typing a relative path' (Get-HintFor $hwnd 'Music\WestieLib') "$HintNotFullPath | red True | moved False"
	Check '  typing a path on a missing drive' (Get-HintFor $hwnd "$($drive):\Music") "$HintNoSuchDrive | red True | moved False"
	Check '  typing the path of a file' (Get-HintFor $hwnd (Resolve-Path $Exe).Path) "$HintNotAFolder | red True | moved False"
	Check '  typing a folder that does not exist' (Get-HintFor $hwnd $MissingFolder) "$HintWillBeCreated | red False | moved False"
	Check '  typing a folder that exists' (Get-HintFor $hwnd $music) ' | red False | moved False'

	[void](Get-HintFor $hwnd 'Music\WestieLib')
	[void][W]::SendMessageW($edit, 0x8, [IntPtr]::Zero, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 200
	Check-FolderInEffect '  leaving it with a red hint keeps the folder' $hwnd 'Music\WestieLib' $DefaultLibrary

	# The click moves the focus out of the field, which commits it, so start with the focus in it
	[W]::Focus($edit)
	[void][W]::PostMessageW($browse, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	$picker = Wait-OwnedWindow $hwnd
	Check '  Browse with that text opens the picker' "$([W]::ClassOf($picker)) $([W]::TextOf($picker))" '#32770 Library folder'
	[void][W]::PostMessageW($picker, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 500
	Check-FolderInEffect '  and Cancel keeps the text and the folder' $hwnd 'Music\WestieLib' $DefaultLibrary

	[void][W]::PostMessageW($browse, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	$boxes = Select-InPicker (Wait-OwnedWindow $hwnd) $music
	Check-FolderInEffect "  picking a folder in the picker (Folder boxes: $boxes)" $hwnd $music $music
	Check '  and its hint' ([W]::TextOf((Get-FolderHint $hwnd))) ''

	Check '  typing a quoted folder that does not exist' (Get-HintFor $hwnd "  `"$MissingFolder\`"  ") "$HintWillBeCreated | red False | moved False"
	[void][W]::SendMessageW($edit, 0x8, [IntPtr]::Zero, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 200
	Check-FolderInEffect '  leaving it: the folder takes effect, cleaned up' $hwnd $MissingFolder $MissingFolder
}

# Tools > Settings, the toolbar's Settings button and Ctrl+Alt+S each open the Settings dialog, centered on the window,
# and Esc, Cancel and its close box each shut it. It shows the folder in effect, read-only, and the version.
function Check-SettingsDialog($hwnd, [string]$folder) {
	$strip = Get-ToolbarStrip $hwnd
	$toolbar = ([W]::Children($strip, 'ToolbarWindow32'))[0]
	$command = [IntPtr][int][W]::CommandOf([W]::GetMenu($hwnd), '&Settings...')
	$opens = @(
		@{ Name = 'Tools > Settings'; Open = { Send-Command $hwnd '&Settings...' } },
		@{ Name = 'the toolbar button'; Open = { [void][W]::PostMessageW($strip, 0x111, $command, $toolbar) } },
		@{ Name = 'Ctrl+Alt+S'; Open = { [W]::PostCtrlKey($hwnd, 0x53, $true) } }
	)
	$closes = @(
		@{ Name = 'Esc'; Close = { param($dialog) [void][W]::PostMessageW($dialog, 0x100, [IntPtr]0x1B, [IntPtr]1) } },
		@{ Name = 'Cancel'; Close = { param($dialog) [void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero) } },
		@{ Name = 'its close box'; Close = { param($dialog) [void][W]::PostMessageW($dialog, 0x10, [IntPtr]::Zero, [IntPtr]::Zero) } }
	)
	for ($i = 0; $i -lt $opens.Count; $i++) {
		& $opens[$i].Open
		$dialog = Wait-OwnedWindow $hwnd
		Check "$($opens[$i].Name) opens Settings" "$([W]::ClassOf($dialog)) $([W]::TextOf($dialog))" '#32770 Settings'
		if ($i -eq 0) {
			$d = New-Object W+RECT
			$o = New-Object W+RECT
			[void][W]::GetWindowRect($dialog, [ref]$d)
			[void][W]::GetWindowRect($hwnd, [ref]$o)
			$offset = "$(($d.Left + $d.Right) - ($o.Left + $o.Right)) $(($d.Top + $d.Bottom) - ($o.Top + $o.Bottom))"
			Check-That '  centered on the window (offset x2)' $offset ($offset -match '^-?[01] -?[01]$') '-1 to 1 each way'
			$edit = ([W]::Children($dialog, 'Edit'))[0]
			$readOnly = ([W]::GetWindowLongW($edit, -16) -band 0x800) -ne 0
			Check '  the folder in effect, read-only' "$([W]::EditText($edit)) $readOnly" "$folder True"
			$version = @([W]::Children($dialog, 'Static') | ForEach-Object { [W]::TextOf($_) } | Where-Object { $_ -like 'WestieLib *' })
			Check '  the version' ($version -join ', ') "WestieLib $((Get-Item $Exe).VersionInfo.ProductVersion)"
			Check '  starts on Change, the path not selected' "$([W]::TextOf([W]::FocusOf($dialog))) $([W]::SendMessageW($edit, 0xB0, [IntPtr]::Zero, [IntPtr]::Zero))" "&Change$([char]0x2026) 0"
			$pattern = "yt-dlp \|  \| Failed: $LookupFailure / ffmpeg \|  \| Failed: $LookupFailure \| auto 1 \| Never checked \| check True"
			$actual = Get-SettingsTools $dialog
			Check-That '  tools, none installed' $actual ($actual -match "^$pattern$") $pattern
			if ($Screenshot) { Save-Screenshot $dialog ("$($Screenshot -replace '\.png$', '')-settings.png") }
		}
		& $closes[$i].Close $dialog
		Wait-Closed $dialog
		Check "  $($closes[$i].Name) closes it" ([W]::IsWindow($dialog)) 'False'
	}
}

# Settings starts from a folder typed into the welcome field but not yet in effect. Change... opens the picker over the
# dialog, and a picked folder shows in the dialog at once but takes effect only on OK.
function Check-SettingsLibraryFolder($hwnd) {
	$music = [Environment]::GetFolderPath('MyMusic')
	$typed = "$MissingFolder-typed"
	$field = Get-FolderEdit $hwnd
	[W]::Focus($field)
	[W]::SetEditText($field, $typed)
	Send-Command $hwnd '&Settings...'
	$dialog = Wait-OwnedWindow $hwnd
	$edit = ([W]::Children($dialog, 'Edit'))[0]
	$change = [W]::GetDlgItem($dialog, 1001)
	Check 'Settings, with a folder typed into the field' ([W]::EditText($edit)) $typed

	[void][W]::PostMessageW($change, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	$picker = Wait-OwnedWindow $dialog
	Check '  Change opens the picker over the dialog' "$([W]::ClassOf($picker)) $([W]::TextOf($picker))" '#32770 Library folder'
	[void][W]::PostMessageW($picker, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $picker
	Check '  closing the picker keeps the folder' ([W]::EditText($edit)) $typed

	[void][W]::PostMessageW($change, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	$boxes = Select-InPicker (Wait-OwnedWindow $dialog) $music
	Check "  a picked folder shows (Folder boxes: $boxes)" ([W]::EditText($edit)) $music
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
	Check-FolderInEffect '  Cancel: the typed folder stays in effect' $hwnd $typed $typed
	Check '  and in settings.json' (Get-SavedSettings) "version 1, $typed"

	Send-Command $hwnd '&Settings...'
	$dialog = Wait-OwnedWindow $hwnd
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1001), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	[void](Select-InPicker (Wait-OwnedWindow $dialog) $music)
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
	Check-FolderInEffect '  OK: the picked folder takes effect' $hwnd $music $music
	Check '  and is saved' (Get-SavedSettings) "version 1, $music"
}

# settings.json's version and library folder, read as UTF-8
function Get-SavedSettings {
	if (-not (Test-Path $SettingsFile)) { return 'no file' }
	$json = Get-Content $SettingsFile -Raw -Encoding UTF8 | ConvertFrom-Json
	"version $($json.version), $($json.libraryPath)"
}

function Get-SavedAutoUpdate {
	if (-not (Test-Path $SettingsFile)) { return 'no file' }
	(Get-Content $SettingsFile -Raw -Encoding UTF8 | ConvertFrom-Json).autoUpdateTools
}

$Invariant = [Globalization.CultureInfo]::InvariantCulture
$CheckedAt = [DateTime]::UtcNow.AddDays(-10)

# What Settings says for a check at $CheckedAt: the local day and month, and the year when it isn't this one
function Get-LastCheckedText {
	$local = $CheckedAt.ToLocalTime()
	$year = if ($local.Year -ne [DateTime]::Now.Year) { " $($local.Year)" } else { '' }
	"Last checked $($local.Day) $($local.ToString('MMM', $Invariant))$year"
}

# Installed tools as the app would leave them: tools.json, checked at $CheckedAt, and the executables: the fake yt-dlp,
# and empty files for ffmpeg, which nothing runs yet
function Install-FakeTools {
	New-Item -ItemType Directory -Force $ToolsDir | Out-Null
	Copy-Item $FakeYtDlp $ToolsDir
	foreach ($file in 'ffmpeg.exe', 'ffprobe.exe') { [IO.File]::WriteAllBytes((Join-Path $ToolsDir $file), [byte[]]@()) }
	$json = '{"version": 1, "checkedAt": "' + $CheckedAt.ToString("yyyy-MM-dd'T'HH:mm:ss'Z'", $Invariant) + '", "tools": {' +
		'"yt-dlp": {"release": "2026.10.21", "digest": "sha256:0"}, "ffmpeg": {"release": "n9.0", "digest": "sha256:0"}}}'
	[IO.File]::WriteAllText((Join-Path $ToolsDir 'tools.json'), $json)
}

# The Tools part's icon color and tooltip lines, matched against a regex
function Check-ToolsPart([string]$what, $hwnd, [string]$pattern) {
	$status = Get-StatusBar $hwnd
	$tip = [W]::TooltipText($status, $ToolsPart) -replace "`r`n", ' / '
	$actual = "$([W]::StatusIconColor($hwnd, $status, $ToolsPart)) | $tip"
	Check-That $what $actual ($actual -match "^$pattern$") $pattern
}

function Set-SavedAutoUpdate([bool]$on) {
	$json = Get-Content $SettingsFile -Raw -Encoding UTF8 | ConvertFrom-Json
	$json.autoUpdateTools = $on
	[IO.File]::WriteAllText($SettingsFile, ($json | ConvertTo-Json), (New-Object Text.UTF8Encoding $false))
}

# The Settings dialog's Tools group: the list's rows, the check box, Last checked, and whether Check for updates is
# enabled
function Get-SettingsTools($dialog) {
	$list = [W]::GetDlgItem($dialog, 1003)
	$count = [int][W]::SendMessageW($list, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero)
	$rows = @(0..($count - 1) | ForEach-Object { [W]::ListViewRow($list, $_) }) -join ' / '
	$auto = [W]::SendMessageW([W]::GetDlgItem($dialog, 1004), 0xF0, [IntPtr]::Zero, [IntPtr]::Zero)
	"$rows | auto $auto | $([W]::TextOf([W]::GetDlgItem($dialog, 1006))) | check $([W]::IsWindowEnabled([W]::GetDlgItem($dialog, 1005)))"
}

# Clicks Check for updates in the dialog and waits for the button to come back: it grays while the run goes
function Invoke-CheckForUpdates($dialog) {
	$button = [W]::GetDlgItem($dialog, 1005)
	[void][W]::PostMessageW($button, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	for ($i = 0; $i -lt 150; $i++) {
		Start-Sleep -Milliseconds 100
		if ($i -ge 2 -and [W]::IsWindowEnabled($button)) { return }
	}
	throw 'Check for updates did not end'
}

# The Tools group with the tools installed and Update tools automatically off, as the run starts them. Check for
# updates runs at once, even with the check box off; its look-ups fail, which leaves the tools' files as they were.
# The check box takes effect on OK, and is saved.
function Check-SettingsTools($hwnd) {
	$checked = Get-LastCheckedText
	Send-Command $hwnd '&Settings...'
	$dialog = Wait-OwnedWindow $hwnd
	Check 'Settings, tools installed' (Get-SettingsTools $dialog) "yt-dlp | 2026.10.21 | Installed / ffmpeg | n9.0 | Installed | auto 0 | $checked | check True"
	Invoke-CheckForUpdates $dialog
	$pattern = "yt-dlp \| 2026\.10\.21 \| Failed: $LookupFailure / ffmpeg \| n9\.0 \| Failed: $LookupFailure \| auto 0 \| $checked \| check True"
	$actual = Get-SettingsTools $dialog
	Check-That '  Check for updates, GitHub unreachable' $actual ($actual -match "^$pattern$") $pattern
	Check-ToolsPart '  Tools icon | tooltip' $hwnd "amber \| yt-dlp: $LookupFailure / ffmpeg: $LookupFailure"
	$kept = @('yt-dlp.exe', 'ffmpeg.exe', 'ffprobe.exe' | ForEach-Object { (Get-Item (Join-Path $ToolsDir $_)).Length }) -join ' '
	Check '  tools kept, no staging' "$kept $(Test-Path (Join-Path $ToolsDir '.staging'))" "$((Get-Item $FakeYtDlp).Length) 0 0 False"
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1004), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 200
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
	Check '  checking Update tools automatically, then Cancel' (Get-SavedAutoUpdate) 'False'

	Send-Command $hwnd '&Settings...'
	$dialog = Wait-OwnedWindow $hwnd
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1004), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 200
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
	Check '  then OK saves it' (Get-SavedAutoUpdate) 'True'
	Send-Command $hwnd '&Settings...'
	$dialog = Wait-OwnedWindow $hwnd
	Check '  and the dialog shows it' ((Get-SettingsTools $dialog) -replace '^.* \| (auto \d) \|.*$', '$1') 'auto 1'
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
}

# Starts the app with a settings.json that isn't JSON: it shows the error instead of its window, exits with 1, and
# leaves the file as it was
function Check-BrokenSettings {
	$broken = '{"version": 1, "libraryPath": '
	[IO.File]::WriteAllText($SettingsFile, $broken)
	$process = Start-Process -FilePath $Exe -PassThru
	$box = [IntPtr]::Zero
	for ($i = 0; $i -lt 50 -and $box -eq [IntPtr]::Zero; $i++) {
		Start-Sleep -Milliseconds 100
		$box = [W]::WindowOfProcess($process.Id, '#32770')
	}
	$message = @([W]::Children($box, 'Static') | ForEach-Object { [W]::TextOf($_) } | Where-Object { $_ -like '*settings.json*' })
	$window = [W]::FindWindowW('WestieLib.MainWindow', [NullString]::Value) -ne [IntPtr]::Zero
	Check 'A broken settings.json: error, names the file, no window' "$([W]::TextOf($box)) $($message.Count) $window" 'WestieLib 1 False'
	[void][W]::PostMessageW($box, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
	if (-not $process.WaitForExit(5000)) { $process.Kill(); throw 'Process did not exit' }
	Check '  exit code, file kept' "$($process.ExitCode) $([IO.File]::ReadAllText($SettingsFile) -eq $broken)" '1 True'
}

$StateFile = Join-Path $DataDir 'state.json'
$SpotifyId = '7mKq2pLxV9nT4cR1sY8wZa'
$SpotifyLink = "https://open.spotify.com/playlist/$SpotifyId"
$SpotifyName = "Late Night Caf$([char]0xE9)"
$SlowSpotifyId = '3k9QeTqH2wVfXbN7RjL0aC'
$MissingSpotifyId = '0aaaaaaaaaaaaaaaaaaaaa'
$BrokenSpotifyId = '0bbbbbbbbbbbbbbbbbbbbb'
$Web.SpotifyNames[$SpotifyId] = $SpotifyName
$Web.SpotifyNames[$SlowSpotifyId] = 'Sunday Social'
[void]$Web.SlowIds.Add($SlowSpotifyId)
[void]$Web.BrokenIds.Add($BrokenSpotifyId)
$YouTubeLink = 'https://music.youtube.com/playlist?list=PLx9k2_-Ab'
$YouTubeName = "Caf$([char]0xE9) Check Mix"
$NotSupported = 'Not a supported playlist link'
$WaitingForTools = "Waiting for required tools to be installed$Ellipsis"
$Unavailable = 'This playlist doesn''t exist or is private.'
$SpotifyFound = "$SpotifyName Spotify playlist"
$YouTubeFound = "$YouTubeName YouTube Music playlist"
# What the Add playlist dialog says under the link once the look-up answered, then the Add button's text and whether it
# is enabled. Run 1 has no yt-dlp, so YouTube Music links wait for it.
$LinkChecks = @(
	@{ Link = ''; Line = ' | Add False' },
	@{ Link = $YouTubeLink; Line = "$WaitingForTools | Add False" },
	@{ Link = "  https://www.youtube.com/playlist?si=x&list=PLx9k2_-Ab`r`n"; Line = "$WaitingForTools | Add False" },
	@{ Link = 'MUSIC.youtube.com/browse/VLPLx9k2_-Ab'; Line = "$WaitingForTools | Add False" },
	@{ Link = 'https://music.youtube.com/playlist?list=LM'; Line = "$NotSupported | Add False" },
	@{ Link = 'https://music.youtube.com/watch?v=abc&list=PLx9k2_-Ab'; Line = "$NotSupported | Add False" },
	@{ Link = "$SpotifyLink`?si=4f1c2e7a9b3d4e5f"; Line = "$SpotifyFound | Add True" },
	@{ Link = "https://open.spotify.com/intl-nl/playlist/$SpotifyId"; Line = "$SpotifyFound | Add True" },
	@{ Link = "spotify:playlist:$SpotifyId"; Line = "$SpotifyFound | Add True" },
	@{ Link = "spotify:playlist:$MissingSpotifyId"; Line = "$Unavailable | Add False" },
	@{ Link = "spotify:playlist:$BrokenSpotifyId"; Line = 'Couldn''t look it up: HTTP 500 from 127.0.0.1 | Add False' },
	@{ Link = "https://open.spotify.com/album/$SpotifyId"; Line = "$NotSupported | Add False" },
	@{ Link = 'https://open.spotify.com/playlist/7mKq2pLx'; Line = "$NotSupported | Add False" },
	@{ Link = 'hello'; Line = "$NotSupported | Add False" }
)

# The line under the link, then the Add button's text and whether it is enabled
function Get-LinkLine($dialog) {
	$add = [W]::GetDlgItem($dialog, 1)
	"$([W]::TextOf([W]::GetDlgItem($dialog, 1101))) | $([W]::TextOf($add)) $([W]::IsWindowEnabled($add))"
}

function Set-PlaylistLink($dialog, [string]$link) {
	[W]::SetEditText([W]::GetDlgItem($dialog, 1100), $link)
	Start-Sleep -Milliseconds 50
}

# The line once the look-up has answered; it starts 300 ms after the last change. The answer sets the line, then the
# button, so this reads them again after a moment.
function Wait-LinkLine($dialog) {
	for ($i = 0; $i -lt 100 -and (Get-LinkLine $dialog) -like 'Looking up*'; $i++) { Start-Sleep -Milliseconds 100 }
	Start-Sleep -Milliseconds 100
	Get-LinkLine $dialog
}

# The toolbar, File > Add playlist, Ctrl+N, the left panel's button and the welcome screen's each open Add playlist,
# centered on the window. It starts empty, with Add disabled and Sync now checked; the line under the link follows the
# typing and the look-ups. Cancel writes no state.json.
function Check-AddPlaylistDialog($hwnd) {
	$strip = Get-ToolbarStrip $hwnd
	$toolbar = ([W]::Children($strip, 'ToolbarWindow32'))[0]
	$command = [IntPtr][int][W]::CommandOf([W]::GetMenu($hwnd), '&Add playlist...')
	$leftButton = ([W]::Children((Get-RowPanels $hwnd)[0], 'Button'))[0]
	$welcomeButton = (Get-DirectChildren (Get-Panel $hwnd 'Welcome') 'Button')[0]
	$opens = @(
		@{ Name = 'the toolbar button'; Open = { [void][W]::PostMessageW($strip, 0x111, $command, $toolbar) } },
		@{ Name = 'File > Add playlist'; Open = { Send-Command $hwnd '&Add playlist...' } },
		@{ Name = 'Ctrl+N'; Open = { [W]::PostCtrlKey($hwnd, 0x4E) } },
		@{ Name = 'the left panel button'; Open = { [void][W]::PostMessageW($leftButton, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero) } },
		@{ Name = 'the welcome button'; Open = { [void][W]::PostMessageW($welcomeButton, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero) } }
	)
	foreach ($open in $opens) {
		& $open.Open
		$dialog = Wait-OwnedWindow $hwnd
		Check "$($open.Name) opens Add playlist" "$([W]::ClassOf($dialog)) $([W]::TextOf($dialog))" '#32770 Add playlist'
		[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
		Wait-Closed $dialog
	}

	Send-Command $hwnd '&Add playlist...'
	$dialog = Wait-OwnedWindow $hwnd
	$d = New-Object W+RECT
	$o = New-Object W+RECT
	[void][W]::GetWindowRect($dialog, [ref]$d)
	[void][W]::GetWindowRect($hwnd, [ref]$o)
	$offset = "$(($d.Left + $d.Right) - ($o.Left + $o.Right)) $(($d.Top + $d.Bottom) - ($o.Top + $o.Bottom))"
	Check-That '  centered on the window (offset x2)' $offset ($offset -match '^-?[01] -?[01]$') '-1 to 1 each way'
	$syncNow = [W]::SendMessageW([W]::GetDlgItem($dialog, 1102), 0xF0, [IntPtr]::Zero, [IntPtr]::Zero)
	Check '  focus on the link | line | Add | Sync now' "$([W]::FocusOf($dialog) -eq [W]::GetDlgItem($dialog, 1100)) | $(Get-LinkLine $dialog) | $syncNow" 'True |  | Add False | 1'
	foreach ($case in $LinkChecks) {
		Set-PlaylistLink $dialog $case.Link
		Check "  '$($case.Link.Trim())'" (Wait-LinkLine $dialog) $case.Line
	}
	Set-PlaylistLink $dialog "spotify:playlist:$SlowSpotifyId"
	Check '  while Spotify answers' (Get-LinkLine $dialog) "Looking up the Spotify playlist$Ellipsis | Add False"
	# The spinner takes a step every 100 ms, unless Windows' animations are off
	$line = [W]::GetDlgItem($dialog, 1101)
	$first = [W]::ChildPixels($dialog, $line, (S 16))
	Start-Sleep -Milliseconds 250
	$moved = if ([W]::ChildPixels($dialog, $line, (S 16)) -ne $first) { 'turns' } else { 'stands still' }
	Check '  its spinner' $moved $(if ([W]::ClientAreaAnimation()) { 'turns' } else { 'stands still' })
	if ($Screenshot) { Save-Screenshot $dialog ("$($Screenshot -replace '\.png$', '')-add-playlist-looking.png") }
	Check '  then' (Wait-LinkLine $dialog) 'Sunday Social Spotify playlist | Add True'
	$logo = [W]::ChildPixels($dialog, $line, (S 16))
	Start-Sleep -Milliseconds 250
	Check '  and its logo stands still' $(if ([W]::ChildPixels($dialog, $line, (S 16)) -eq $logo) { 'still' } else { 'moves' }) 'still'
	Set-PlaylistLink $dialog $YouTubeLink
	if ($Screenshot) {
		Start-Sleep -Milliseconds 300
		Save-Screenshot $dialog ("$($Screenshot -replace '\.png$', '')-add-playlist-waiting.png")
	}
	# Shared links are the longest: a Spotify link with its ?si= part, and a YouTube Music list id of 34 characters
	$edit = [W]::GetDlgItem($dialog, 1100)
	$c = New-Object W+RECT
	[void][W]::GetClientRect($edit, [ref]$c)
	$margins = [int64][W]::SendMessageW($edit, 0xD4, [IntPtr]::Zero, [IntPtr]::Zero)
	$room = $c.Right - ($margins -band 0xFFFF) - (($margins -shr 16) -band 0xFFFF)
	foreach ($link in "$SpotifyLink`?si=4f1c2e7a9b3d4e5f", 'https://music.youtube.com/playlist?list=PLWmMwQkx8mKVm2wQgkWMmwMWqWZqWmWZW') {
		$width = [W]::TextWidth($link, $script:dpi)
		Check-That "  fits without scrolling: $($link.Substring(8, 20))..." "$width of $room" ($width -le $room) "at most $room"
	}
	Set-PlaylistLink $dialog "$SpotifyLink`?si=4f1c2e7a9b3d4e5f"
	Check '  a new playlist again, for the screenshot' (Wait-LinkLine $dialog) "$SpotifyFound | Add True"
	if ($Screenshot) {
		Start-Sleep -Milliseconds 300
		Save-Screenshot $dialog ("$($Screenshot -replace '\.png$', '')-add-playlist.png")
	}
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
	Check '  Cancel writes no state.json' (Test-Path $StateFile) 'False'
}

# The playlists in state.json, a file named here, as "key url [name] whether addedAt is an ISO 8601 time"
function Get-SavedPlaylists([string]$file = $StateFile) {
	if (-not (Test-Path $file)) { return 'no file' }
	$json = Get-Content $file -Raw -Encoding UTF8 | ConvertFrom-Json
	@($json.playlists.PSObject.Properties | ForEach-Object {
		"$($_.Name) $($_.Value.url) [$($_.Value.name)] $($_.Value.addedAt -match '^\d{4}-\d\d-\d\dT\d\d:\d\d:\d\dZ$')"
	}) -join ' / '
}

# Adds a playlist through File > Add playlist; returns the dialog's line for the link
function Add-Playlist($hwnd, [string]$link) {
	Send-Command $hwnd '&Add playlist...'
	$dialog = Wait-OwnedWindow $hwnd
	Set-PlaylistLink $dialog $link
	$line = Wait-LinkLine $dialog
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
	$line
}

# Welcome screen and tracks panel visible; the left panel's "No playlists yet." and the inspector's placeholder visible
function Get-PlaylistScreens($hwnd) {
	$empty = ([W]::Children((Get-RowPanels $hwnd)[0], 'WestieLib.Label'))[0]
	$placeholder = ([W]::Children((Get-Panel $hwnd 'Inspector'), 'WestieLib.Label'))[0]
	"welcome $([W]::IsWindowVisible((Get-Panel $hwnd 'Welcome'))), tracks $([W]::IsWindowVisible((Get-Panel $hwnd 'Tracks'))), " +
		"no playlists $([W]::IsWindowVisible($empty)), placeholder $([W]::IsWindowVisible($placeholder))"
}

$WithPlaylists = 'welcome False, tracks True, no playlists False, placeholder False'

function Get-PlaylistList($hwnd) { ([W]::Children((Get-RowPanels $hwnd)[0], 'WestieLib.PlaylistList'))[0] }

# Border, padding, the name line, three lines under it, padding, border
function Get-PlaylistRowHeight { 2 + (S 5) + (S 18) + 3 * (S 16) + (S 6) }

# Where the Add playlist button starts in the list, scrolled to the top, under $count rows 1 apart
function Get-ListButtonTop([int]$count) { (S 1) + $count * ((Get-PlaylistRowHeight) + (S 1)) + (S 8) }

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

function Get-AllTracksRow($hwnd) { ([W]::Children((Get-RowPanels $hwnd)[0], 'WestieLib.AllTracksRow'))[0] }

# Where row $index (from 1) starts in the list, as it is scrolled now
function Get-PlaylistRowTop($hwnd, [int]$index) {
	(S 1) + ($index - 1) * ((Get-PlaylistRowHeight) + (S 1)) - [W]::GetScrollPos((Get-PlaylistList $hwnd), 1)
}

# What paints the selection box, sampled just inside each box's top-left corner: All tracks, and the rows from 1 of the
# $count in the list that are in view
function Get-Selection($hwnd, [int]$count) {
	$list = Get-PlaylistList $hwnd
	$height = Get-ClientHeight $list
	$selected = @()
	if (([W]::PixelsIn($hwnd, (Get-AllTracksRow $hwnd), @((S 4), (S 4))))[0] -eq 'cce8ff') { $selected += 'All tracks' }
	$rows = @(if ($count -gt 0) { 1..$count | Where-Object { $y = (Get-PlaylistRowTop $hwnd $_) + 3; $y -ge 0 -and $y -lt $height } })
	$points = @($rows | ForEach-Object { (S 3) + 3; (Get-PlaylistRowTop $hwnd $_) + 3 })
	if ($rows.Count -gt 0) {
		$colors = [W]::PixelsIn($hwnd, $list, [int[]]$points)
		for ($i = 0; $i -lt $rows.Count; $i++) { if ($colors[$i] -eq 'cce8ff') { $selected += "row $($rows[$i])" } }
	}
	if ($selected.Count -eq 0) { 'none' } else { $selected -join ', ' }
}

# Whether the commands that need a selected playlist are enabled
function Get-PlaylistCommands($hwnd) {
	Update-MenuStates $hwnd
	$menu = [W]::GetMenu($hwnd)
	$states = '&Remove playlist...', 'Sync &selected playlist' | ForEach-Object { ([W]::GetMenuState($menu, [W]::CommandOf($menu, $_), 0) -band 0x1) -eq 0 }
	"Remove $($states[0]), Sync selected $($states[1])"
}

function Send-Click($window, [int]$x, [int]$y) {
	[void][W]::SendMessageW($window, 0x201, [IntPtr]1, (LParam $x $y))
	[void][W]::SendMessageW($window, 0x202, [IntPtr]0, (LParam $x $y))
}

function Click-PlaylistRow($hwnd, [int]$index) {
	Send-Click (Get-PlaylistList $hwnd) (S 20) ((Get-PlaylistRowTop $hwnd $index) + [int]((Get-PlaylistRowHeight) / 2))
}

function Click-AllTracks($hwnd) { Send-Click (Get-AllTracksRow $hwnd) (S 20) (S 10) }

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

# A state.json with $count playlists (at most 31), added a day apart: Spotify and YouTube Music in turn, every third with a name
# too long for its row
function Write-ManyPlaylists([int]$count) {
	$entries = 1..$count | ForEach-Object {
		$key = if ($_ % 2) { "spotify:P$('{0:D21}' -f $_)" } else { "ytm:PLcheck$_" }
		$url = if ($_ % 2) { "https://open.spotify.com/playlist/$($key.Substring(8))" } else { "https://music.youtube.com/playlist?list=$($key.Substring(4))" }
		$name = if ($_ % 3 -eq 0) { "Playlist $_ with a name too long to fit in its row" } else { '' }
		$added = '2026-01-{0:D2}T12:00:00Z' -f $_
		"`"$key`": {`"url`": `"$url`", `"name`": `"$name`", `"addedAt`": `"$added`"}"
	}
	[IO.File]::WriteAllText($StateFile, '{"version": 1, "playlists": {' + ($entries -join ', ') + '}}')
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

# With yt-dlp installed, a YouTube Music link is looked up: the answer names the playlist, or says it is missing,
# private or out of reach. The look-up waits 300 ms after the last change, so quick changes run yt-dlp once, with the
# arguments the app gives it.
function Check-YouTubeLookups($hwnd) {
	Send-Command $hwnd '&Add playlist...'
	$dialog = Wait-OwnedWindow $hwnd
	Set-PlaylistLink $dialog $YouTubeLink
	Check 'yt-dlp installed: a YouTube Music link' (Wait-LinkLine $dialog) "$YouTubeFound | Add True"
	$cases = @(
		@{ Id = 'PLmissing1'; Line = "$Unavailable | Add False" },
		@{ Id = 'PLprivate1'; Line = "$Unavailable | Add False" },
		@{ Id = 'PLbroken1'; Line = 'Couldn''t look it up: Unable to download API page: HTTP Error 503: Service Unavailable | Add False' }
	)
	foreach ($case in $cases) {
		Set-PlaylistLink $dialog "https://music.youtube.com/playlist?list=$($case.Id)"
		Check "  $($case.Id)" (Wait-LinkLine $dialog) $case.Line
	}
	Set-PlaylistLink $dialog 'https://music.youtube.com/playlist?list=PLslow1'
	Check '  while yt-dlp looks it up' (Get-LinkLine $dialog) "Looking up the YouTube Music playlist$Ellipsis | Add False"
	Check '  then' (Wait-LinkLine $dialog) 'Checked PLslow1 YouTube Music playlist | Add True'
	$runs = @(Get-Content $FakeYtDlpRuns -Encoding UTF8).Count
	foreach ($id in 'PLquick1', 'PLquick2', 'PLquick3') { [W]::SetEditText([W]::GetDlgItem($dialog, 1100), "https://music.youtube.com/playlist?list=$id") }
	Check '  three quick changes' (Wait-LinkLine $dialog) 'Checked PLquick3 YouTube Music playlist | Add True'
	$new = @(Get-Content $FakeYtDlpRuns -Encoding UTF8 | Select-Object -Skip $runs)
	Check '  run yt-dlp once, with these arguments' ($new -join ' / ') '--ignore-config|--flat-playlist|--dump-single-json|--playlist-items|0|https://music.youtube.com/playlist?list=PLquick3'
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
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
	$placeholder = [W]::RectIn(([W]::Children((Get-RowPanels $hwnd)[2], 'WestieLib.Label'))[0], $hwnd)
	$band = New-Object W+RECT
	$band.Left = $inspector.Left - (S 200)
	$band.Right = $inspector.Right
	$band.Top = $placeholder.Top
	$band.Bottom = $placeholder.Bottom
	Check 'Inspector drag: placeholder never blinks' ([W]::WatchDrag($hwnd, $band, (Get-Splitter $hwnd 1), -(S 200), $true)) 'blinked 0'
}

# The inspector's placeholder fills the panel's width inside the padding, its center within a pixel of the padded
# area's
function Check-InspectorPlaceholder([string]$what, $hwnd) {
	$panel = (Get-RowPanels $hwnd)[2]
	$label = ([W]::Children($panel, 'WestieLib.Label'))[0]
	$r = [W]::RectIn($label, $panel)
	$sides = "$($r.Left) $((Get-ClientWidth $panel) - $r.Right)"
	# Twice the offset, so it stays whole
	$offset = ($r.Top + $r.Bottom) - ((S 12) + (Get-ClientHeight $panel) - (S 10))
	$text = [W]::TextOf($label) -replace "`n", ' / '
	$ok = $text -eq 'Add a playlist to see / its tracks here.' -and $sides -eq "$(S 12) $(S 12)" -and [math]::Abs($offset) -le 2
	Check-That $what "$text | $sides | $offset" $ok "the text | $(S 12) $(S 12) | -2 to 2"
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
	Check 'A start writes no settings.json' (Get-SavedSettings) 'no file'
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
	Check 'Grayed while idle, no playlists, nothing selected' ([W]::ItemsWithState($menu, 0x1)) ('&Remove playlist..., Sync &all, Sync &selected playlist, &Cancel, ' +
		'&Confirm match, C&hoose video..., &Edit track..., &Retry, &Keep, &Move to Recycle Bin..., &Show in folder, Open on &YouTube, Open source &page')
	Check-ToolsPart 'No tools, GitHub unreachable: Tools icon | tooltip' $h "amber \| yt-dlp: $LookupFailure / ffmpeg: $LookupFailure"
	Check 'Checked items' ([W]::ItemsWithState($menu, 0x8)) ''
	Send-Command $h '&About WestieLib'
	$about = Wait-OwnedWindow $h
	Check 'Help > About opens a dialog' "$([W]::ClassOf($about)) $([W]::TextOf($about))" '#32770 About WestieLib'
	[void][W]::PostMessageW($about, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 300
	Check '  and closes' ([W]::IsWindow($about)) 'False'
	Check-SettingsDialog $h $DefaultLibrary
	Check '  and writes no settings.json' (Get-SavedSettings) 'no file'
	Check-MenuBarEdge 'Menu bar runs into the toolbar' $h -Menus
	Check 'Initial window size' ((Get-Bounds $h) -replace '^.* ', '') "$(S 1280)x$(S 800)"
	Check-Bars 'Toolbar strip | status bar' $h
	$toolbar = ([W]::Children((Get-ToolbarStrip $h), 'ToolbarWindow32'))[0]
	Check 'Toolbar buttons, no playlists' ([W]::ToolbarButtons($toolbar)) 'Add playlist, Sync all v (disabled), Cancel (disabled), -, Settings'
	Check-Toolbar ' ' $h
	Check-StatusParts 'Status bar part edges' $h
	Check-StatusTexts ' ' $h
	$cw = Get-ClientWidth $h
	$g = S 5
	Check-Widths 'Initial widths' $h @((S 250), ($cw - (S 250) - (S 290) - 2 * $g), (S 290))
	Check-PlaylistPanel 'Left panel:' $h
	Check-InspectorPlaceholder 'Inspector placeholder | sides | center offset x2' $h
	Check-Welcome 'Welcome screen' $h
	Check-LibraryFolder $h
	Check '  saved in settings.json' (Get-SavedSettings) "version 1, $MissingFolder"
	Check-SettingsLibraryFolder $h
	Check 'Drag steps left unpainted' (Measure-UnpaintedDragSteps $h) 0
	Check-NoBlinking $h
	Move-Splitter $h 0 0
	Move-Splitter $h 0 (S 400)
	Check-Widths 'Playlists splitter to 400' $h @((S 400), ($cw - (S 400) - (S 290) - 2 * $g), (S 290))
	Move-Splitter $h 0 5000
	Check-Widths 'Playlists far right: tracks stops at 400' $h @(($cw - (S 290) - 2 * $g - (S 400)), (S 400), (S 290)) 1
	Move-Splitter $h 0 -100
	Check-Widths 'Playlists far left: stops at 250' $h @((S 250), ($cw - (S 250) - (S 290) - 2 * $g), (S 290))
	Move-Splitter $h 1 ($cw - $g - (S 500))
	Check-Widths 'Inspector splitter for a 500 inspector' $h @((S 250), ($cw - (S 250) - (S 500) - 2 * $g), (S 500))
	Check-InspectorPlaceholder '  placeholder | sides | center offset x2' $h
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
	Check-InspectorPlaceholder '  placeholder | sides | center offset x2' $h
	Check-AddPlaylistDialog $h
	[W]::SetEditText((Get-FolderEdit $h), $ClosedFolder)
	"File > Exit, with a folder typed but the field never left"
	Stop-App $app -ExitCommand
	Check '  the folder is saved, in UTF-8' (Get-SavedSettings) "version 1, $ClosedFolder"
	$saved = Get-ItemProperty $KeyPath
	Check '  saved bounds' "$($saved.Left),$($saved.Top),$($saved.Right),$($saved.Bottom)" "100,100,$(100 + (S 1200)),$(100 + (S 750))"
	Check '  saved maximized, widths (DIPs)' "$($saved.Maximized) $($saved.PlaylistsWidthDips) $($saved.InspectorWidthDips)" '0 250 500'
	Check '  saved log visible, height (DIPs)' "$($saved.LogVisible) $($saved.LogHeightDips)" '1 200'

	"--- Run 2: restore a normal window, with tools installed, not updated automatically"
	Install-FakeTools
	Set-SavedAutoUpdate $false
	$app = Start-App
	$h = $app.Hwnd
	$cw = Get-ClientWidth $h
	Check-ToolsPart 'Tools installed: Tools icon | tooltip' $h 'green \| yt-dlp 2026\.10\.21 / ffmpeg n9\.0'
	$toolbar = ([W]::Children((Get-ToolbarStrip $h), 'ToolbarWindow32'))[0]
	Check '  toolbar buttons: Sync all still waits for a playlist' ([W]::ToolbarButtons($toolbar)) 'Add playlist, Sync all v (disabled), Cancel (disabled), -, Settings'
	Check-SettingsTools $h
	Check-YouTubeLookups $h
	Check 'Restored bounds' (Get-Bounds $h) "100,100 $(S 1200)x$(S 750)"
	Check-Widths 'Restored widths' $h @((S 250), ($cw - (S 250) - (S 500) - 2 * $g), (S 500))
	Check-Bars '  toolbar strip | status bar' $h
	Check-Log 'Restored log' $h (S 200)
	Check-FolderInEffect 'Restored library folder' $h $ClosedFolder $ClosedFolder
	if ($Screenshot) { Save-Screenshot $h $Screenshot }
	Send-Command $h '&Log'
	Start-Sleep -Milliseconds 200
	Check 'View > Log hides it' (Get-LogVisibility $h) 'False False'
	[void][W]::ShowWindow($h, 3)
	Start-Sleep -Milliseconds 300
	[W]::SetEditText((Get-FolderEdit $h), 'Music\WestieLib')
	"Closed with a relative path typed"
	Stop-App $app
	$saved = Get-ItemProperty $KeyPath
	Check '  saved maximized, log visible' "$($saved.Maximized) $($saved.LogVisible)" '1 0'
	Check '  the relative path is not saved' (Get-SavedSettings) "version 1, $ClosedFolder"

	"--- Run 3: restore a maximized window"
	$written = (Get-Item $SettingsFile).LastWriteTimeUtc
	$app = Start-App
	$h = $app.Hwnd
	Check 'Library folder still the saved one' ([W]::EditText((Get-FolderEdit $h))) $ClosedFolder
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
	Check '  settings.json untouched without a change' ((Get-Item $SettingsFile).LastWriteTimeUtc -eq $written) 'True'

	"--- Run 4: closed while minimized"
	$app = Start-App
	$h = $app.Hwnd
	Check 'Comes back maximized, not minimized' "$([W]::IsZoomed($h)) $([W]::IsIconic($h))" 'True False'
	Stop-App $app

	"--- Run 5: settings.json doesn't read"
	Check-BrokenSettings
	Remove-Item $SettingsFile

	"--- Run 6: adding playlists"
	$app = Start-App
	$h = $app.Hwnd
	Check 'No playlists yet: screens' (Get-PlaylistScreens $h) 'welcome True, tracks False, no playlists True, placeholder True'
	Check-AddingPlaylists $h
	Stop-App $app

	"--- Run 7: playlists kept"
	$app = Start-App
	$h = $app.Hwnd
	Check 'Screens' (Get-PlaylistScreens $h) $WithPlaylists
	Check-PlaylistRows 'Two rows:' $h 2
	Check-Selection $h
	Check-RowMouse $h
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
	Stop-App $app

	"--- Run 8: more playlists than fit"
	$many = 20
	Write-ManyPlaylists $many
	$app = Start-App
	$h = $app.Hwnd
	# Maximized since run 4, the window would lie under the real cursor, whose moves reach a splitter that has the
	# capture during a drag. The inspector is still 500 wide from run 1, so the window leaves the main column room to
	# give the drag. The screen shows the restore animation for a while, which the drag's captures would take for blinking.
	[void][W]::ShowWindow($h, 9)
	Set-Bounds $h 100 100 (S 1600) (S 750)
	Start-Sleep -Milliseconds 800
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
	$left = New-Object W+RECT
	$left.Right = S 150
	$left.Top = Get-RowTop $h
	$left.Bottom = [W]::RectIn((Get-RowPanels $h)[0], $h).Bottom
	Check 'Playlists drag: rows never blink' ([W]::WatchDrag($h, $left, (Get-Splitter $h 0), (S 300), $false)) 'blinked 0'
	Stop-App $app

	"--- Run 9: state.json doesn't read"
	Check-BrokenState
	Remove-Item $StateFile, "$StateFile.bak"

	"--- Run 10: yt-dlp installs while Add playlist waits for it"
	Remove-Item $ToolsDir -Recurse -Force
	$Web.YtDlp = [IO.File]::ReadAllBytes($FakeYtDlp)
	$env:WESTIELIB_GITHUB_API = $Web.Url
	try {
		$app = Start-App -DuringToolRun
		$h = $app.Hwnd
		Send-Command $h '&Add playlist...'
		$dialog = Wait-OwnedWindow $h
		Set-PlaylistLink $dialog $YouTubeLink
		Check 'yt-dlp downloading: the line waits' (Get-LinkLine $dialog) "$WaitingForTools | Add False"
		[void]$Web.AssetGate.Set()
		for ($i = 0; $i -lt 100 -and (Get-LinkLine $dialog) -match '^(Waiting|Looking up)'; $i++) { Start-Sleep -Milliseconds 100 }
		Start-Sleep -Milliseconds 100
		Check '  installed: the look-up starts by itself' (Get-LinkLine $dialog) "$YouTubeFound | Add True"
		[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
		Wait-Closed $dialog
		Wait-ToolsIdle $h
		Check '  yt-dlp installed' (Get-Item (Join-Path $ToolsDir 'yt-dlp.exe')).Length (Get-Item $FakeYtDlp).Length
		Stop-App $app
	} finally {
		$env:WESTIELIB_GITHUB_API = 'http://127.0.0.1:9'
		$Web.YtDlp = $null
	}

	"--- Run 11: moved to a monitor with another DPI"
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

		"--- Run 12: restored onto that monitor"
		$app = Start-App
		$h = $app.Hwnd
		"DPI $script:dpi"
		Check-Scaled $h
		Stop-App $app
	}
} finally {
	if ($app -and -not $app.Process.HasExited) { $app.Process.Kill() }
	$Web.Stop()
	Remove-Item 'HKCU:\Software\WestieLib' -Recurse -Confirm:$false -ErrorAction SilentlyContinue
	if ($userState) {
		New-Item $KeyPath -Force | Out-Null
		foreach ($name in $StateNames) {
			if ($null -ne $userState.$name) { New-ItemProperty $KeyPath -Name $name -PropertyType DWord -Value $userState.$name | Out-Null }
		}
		"Put the user's saved window state back"
	}
	Remove-Item $DataDir -Recurse -Force -ErrorAction SilentlyContinue
}

"--- $($script:failures) failure(s)"
exit $script:failures
