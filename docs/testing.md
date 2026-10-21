# Testing

## `tests/ui/ui-checks.ps1`

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/ui/ui-checks.ps1 -Exe $build\WestieLib.exe -Screenshot <png>
```

- Starts the exe several times and drives it with Win32 calls through P/Invoke (`Add-Type` C#): menus and commands,
  shortcuts, the toolbar and status bar, window sizes, splitter drags, the log pane, painting, and saved state across
  restarts. The check names in the output say what each one covers.
- Prints `ok` or `FAIL` per check. The exit code is the number of failures.
- `-Screenshot` saves the window to a PNG via `PrintWindow(PW_RENDERFULLCONTENT)`, cropped to
  `DWMWA_EXTENDED_FRAME_BOUNDS`. Read the PNG to look at it.
- Expected sizes are written in DIPs and scaled with the window's DPI (the `S` helper).
- The window shows on the user's screen while it runs.
- Extend it when the UI changes, with checks on rects, registry values and messages.
- Messages sent straight to a window don't exercise real mouse input (cursor shape, mouse capture), so those need a
  try by hand.

## Protect the user's state

- The app writes `HKCU\Software\WestieLib\Window` every time it closes. Any script that starts it must set the user's
  values aside first and put them back in a `finally` block, as `ui-checks.ps1` does. Never delete that key without
  restoring it. When the app saves a new value, add its name to `$StateNames`, or it won't be put back.
- Refuse to run while WestieLib is open, since `FindWindowW` would find the user's window. Kill only the process the
  script started.

## Driving the app from PowerShell

- `Add-Type` in Windows PowerShell fails on a warning about a stale Visual Studio path in `LIB`; set
  `$env:LIB = $null` first.
- PowerShell passes `$null` as `""` to string parameters. Use `[NullString]::Value` (for example as `FindWindowW`'s
  title).
- Call `SetThreadDpiAwarenessContext(-4)` first, or rects come back scaled for a DPI-unaware process.
- To drag a splitter, send `WM_LBUTTONDOWN`, `WM_MOUSEMOVE` and `WM_LBUTTONUP` to the splitter window, with `lparam`
  in splitter client coordinates (`Move-Splitter`, `Move-LogSplitter`).
- The log pane is a `WestieLib.Panel` too, and its splitter a `WestieLib.Splitter`. `EnumChildWindows` also returns
  hidden windows. So the helpers find the panel row by its top edge (`Get-RowPanels`), tell splitters apart by shape
  (`Get-ColumnSplitters`, `Get-LogSplitter`), and skip what isn't visible.
- `PostCtrlKey` presses a Ctrl shortcut without real input, so nothing can reach the user's other windows: it attaches
  to the app's input thread, sets Ctrl in the shared key state, and posts the key's `WM_KEYDOWN`. The app's
  `TranslateAccelerator` sees Ctrl held, as with a real key press.
- The app sets menu item states when a popup opens, so send `WM_INITMENUPOPUP` before reading them
  (`Update-MenuStates`). `Send-Command` looks a command up by its menu text, so the scripts don't repeat IDs.
- Common controls don't marshal pointers across processes. A message that fills a buffer (`TB_GETBUTTON`,
  `SB_GETTEXTW`, …) needs the buffer in the app's process: use `SendForBuffer`.
- Text the app draws itself (the owner-drawn status bar parts) can't be read back with messages. The checks find it
  in a `PrintWindow` capture instead, as the columns darker than mid-gray (`StatusTextGaps`). `StatusSeparators` finds
  the bar's separator lines the same way. `Check-That` is for checks that pass within a range.
- Keep the scripts ASCII. Windows PowerShell reads a `.ps1` without a BOM as ANSI, so a `…` in it turns into
  mojibake. In embedded C#, write such characters as `(char)0x2026`.
