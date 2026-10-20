# Testing

## `tests/ui/ui-checks.ps1`

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/ui/ui-checks.ps1 -Exe $build\WestieLib.exe -Screenshot <png>
```

- Starts the exe several times and drives it with Win32 calls through P/Invoke (`Add-Type` C#): window sizes,
  splitter drags, painting, and saved state across restarts. The check names in the output say what each one
  covers.
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
  restoring it.
- Refuse to run while WestieLib is open, since `FindWindowW` would find the user's window. Kill only the process the
  script started.

## Driving the app from PowerShell

- `Add-Type` in Windows PowerShell fails on a warning about a stale Visual Studio path in `LIB`; set
  `$env:LIB = $null` first.
- PowerShell passes `$null` as `""` to string parameters. Use `[NullString]::Value` (for example as `FindWindowW`'s
  title).
- Call `SetThreadDpiAwarenessContext(-4)` first, or rects come back scaled for a DPI-unaware process.
- To drag a splitter, send `WM_LBUTTONDOWN`, `WM_MOUSEMOVE` and `WM_LBUTTONUP` to the splitter window, with `lparam`
  in splitter client coordinates (`Move-Splitter`).
