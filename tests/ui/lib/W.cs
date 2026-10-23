// Win32 calls for the checks: P/Invoke declarations, and helpers that read a window's state, text and pixels from
// another process

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
		byte[] answer;
		return SendForText(h, msg, w, structSize, build, out answer);
	}

	// The same, also returning the struct as the window left it
	static string SendForText(IntPtr h, uint msg, IntPtr w, int structSize, Func<long, byte[]> build, out byte[] answer) {
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
			answer = new byte[structSize];
			ReadProcessMemory(process, remote, answer, (IntPtr)structSize, out done);
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
	// The same tool's rect in `window`, from TTM_GETTOOLINFOW, which leaves it at 24; an empty rect when no tooltip has it
	public static RECT ToolRect(IntPtr window, int id) {
		int pid, owner;
		GetWindowThreadProcessId(window, out pid);
		var tooltips = new List<IntPtr>();
		EnumWindows((h, l) => {
			GetWindowThreadProcessId(h, out owner);
			if (owner == pid && ClassOf(h) == "tooltips_class32") tooltips.Add(h);
			return true;
		}, IntPtr.Zero);
		foreach (var tooltip in tooltips) {
			byte[] answer;
			var found = SendForText(tooltip, 0x435, IntPtr.Zero, 72, text => {
				var info = new byte[72];
				BitConverter.GetBytes(72).CopyTo(info, 0);
				BitConverter.GetBytes((long)window).CopyTo(info, 8);
				BitConverter.GetBytes((long)id).CopyTo(info, 16);
				BitConverter.GetBytes(text).CopyTo(info, 48);
				return info;
			}, out answer);
			if (found.Length > 0) {
				return new RECT { Left = BitConverter.ToInt32(answer, 24), Top = BitConverter.ToInt32(answer, 28), Right = BitConverter.ToInt32(answer, 32), Bottom = BitConverter.ToInt32(answer, 36) };
			}
		}
		return new RECT();
	}

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

	// A list view's columns in display order, each as its title and width, then " ^" or " v" for the sort arrow,
	// joined with " | ". HDITEMW on x64: mask at 0, cxy at 4, pszText at 8, cchTextMax at 24, fmt at 28, 72 bytes.
	public static string ListViewColumns(IntPtr list) {
		var header = SendMessageW(list, 0x101F, IntPtr.Zero, IntPtr.Zero);
		int count = (int)SendMessageW(header, 0x1200, IntPtr.Zero, IntPtr.Zero);
		long result;
		var order = SendForBuffer(list, 0x103B, (IntPtr)count, count * 4, out result);
		var columns = new List<string>();
		for (int i = 0; i < count; i++) {
			byte[] item;
			var title = SendForText(header, 0x120B, (IntPtr)BitConverter.ToInt32(order, i * 4), 72, text => {
				var request = new byte[72];
				// HDI_WIDTH | HDI_TEXT | HDI_FORMAT
				BitConverter.GetBytes(0x7).CopyTo(request, 0);
				BitConverter.GetBytes(text).CopyTo(request, 8);
				BitConverter.GetBytes(1024).CopyTo(request, 24);
				return request;
			}, out item);
			int format = BitConverter.ToInt32(item, 28);
			string arrow = (format & 0x400) != 0 ? " ^" : (format & 0x200) != 0 ? " v" : "";
			columns.Add(title + " " + BitConverter.ToInt32(item, 4) + arrow);
		}
		return String.Join(" | ", columns);
	}

	// A tab control's tab texts, joined with " | ". TCITEMW on x64: mask at 0, pszText at 16, cchTextMax at 24, 40 bytes.
	public static string TabTexts(IntPtr tabs) {
		int count = (int)SendMessageW(tabs, 0x1304, IntPtr.Zero, IntPtr.Zero);
		var texts = new List<string>();
		for (int i = 0; i < count; i++) {
			texts.Add(SendForText(tabs, 0x133C, (IntPtr)i, 40, text => {
				var request = new byte[40];
				BitConverter.GetBytes(0x1).CopyTo(request, 0);
				BitConverter.GetBytes(text).CopyTo(request, 16);
				BitConverter.GetBytes(1024).CopyTo(request, 24);
				return request;
			}));
		}
		return String.Join(" | ", texts);
	}

	// EM_GETCUEBANNER takes its buffer in wparam and its size in lparam
	public static string CueBanner(IntPtr edit) {
		const int Chars = 256;
		int pid;
		GetWindowThreadProcessId(edit, out pid);
		IntPtr process = OpenProcess(0x38, false, pid);
		IntPtr remote = VirtualAllocEx(process, IntPtr.Zero, (IntPtr)(Chars * 2), 0x3000, 0x04);
		try {
			SendMessageW(edit, 0x1502, remote, (IntPtr)Chars);
			var buffer = new byte[Chars * 2];
			IntPtr read;
			ReadProcessMemory(process, remote, buffer, (IntPtr)buffer.Length, out read);
			return Utf16(buffer);
		} finally {
			VirtualFreeEx(process, remote, IntPtr.Zero, 0x8000);
			CloseHandle(process);
		}
	}

	// Sends a message that fills a RECT in lparam
	public static RECT SendForRect(IntPtr h, uint msg, IntPtr w) {
		long result;
		var bytes = SendForBuffer(h, msg, w, 16, out result);
		return new RECT { Left = BitConverter.ToInt32(bytes, 0), Top = BitConverter.ToInt32(bytes, 4), Right = BitConverter.ToInt32(bytes, 8), Bottom = BitConverter.ToInt32(bytes, 12) };
	}

	// A combo box's items, joined with " | ". Windows marshals CB_GETLBTEXT, as it does every message below WM_USER.
	public static string ComboItems(IntPtr combo) {
		int count = (int)SendMessageW(combo, 0x146, IntPtr.Zero, IntPtr.Zero);
		var items = new List<string>();
		for (int i = 0; i < count; i++) {
			var text = new StringBuilder(256);
			SendText(combo, 0x148, (IntPtr)i, text);
			items.Add(text.ToString());
		}
		return String.Join(" | ", items);
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

	// The size of text in the message font at a DPI, the font the app gives its controls
	static SIZE TextExtent(string text, uint dpi) {
		var metrics = new NONCLIENTMETRICSW();
		metrics.Size = Marshal.SizeOf(metrics);
		SystemParametersInfoForDpi(0x29, (uint)metrics.Size, ref metrics, 0, dpi);
		IntPtr font = CreateFontIndirectW(ref metrics.MessageFont), dc = CreateCompatibleDC(IntPtr.Zero);
		try {
			SelectObject(dc, font);
			SIZE size;
			GetTextExtentPoint32W(dc, text, text.Length, out size);
			return size;
		} finally {
			DeleteDC(dc);
			DeleteObject(font);
		}
	}

	public static int TextWidth(string text, uint dpi) { return TextExtent(text, dpi).Cx; }

	// The message font's line height (tmHeight)
	public static int TextHeight(uint dpi) { return TextExtent("A", dpi).Cy; }

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

	[DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr h, ref POINT p);
	[DllImport("user32.dll", EntryPoint = "SendMessageW")] static extern IntPtr SendRect(IntPtr h, uint msg, IntPtr w, ref RECT r);

	// Where an edit's text starts, from the top of its window: its client area's top, then its formatting rectangle's
	// (EM_GETRECT, which Windows marshals, as every message below WM_USER)
	public static int EditTextTop(IntPtr edit) {
		RECT window, format = new RECT();
		GetWindowRect(edit, out window);
		var client = new POINT();
		ClientToScreen(edit, ref client);
		SendRect(edit, 0xB2, IntPtr.Zero, ref format);
		return client.Y - window.Top + format.Top;
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
