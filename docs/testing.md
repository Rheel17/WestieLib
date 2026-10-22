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
- When a monitor with another DPI is attached, runs 11 and 12 cover it. Run 11 moves the window there, which sends
  `WM_DPICHANGED`, and run 12 restores it there from the saved state. Both check everything that scales
  (`Check-Scaled`).
- Usually every monitor runs at the same scale, and those runs print `Skipped`. After a change to DPI handling (fonts,
  image lists, bar heights, anything in `apply_dpi`), ask the user to set one monitor to another scale (Settings ›
  System › Display) for a run. A script must never change the scaling itself, because that's a system setting.
- The toolbar check compares each button's width with its text measured in the message font at the window's DPI. A
  text measured in an older font shows up as a button that's too narrow.
- The window shows on the user's screen while it runs. A click in another window during a run takes the mouse capture
  from a splitter halfway through a drag. Every later drag then fails with the playlists at their widest, and the blink
  checks say the splitter didn't follow. Run it again without touching the mouse.
- Extend it when the UI changes, with checks on rects, registry values and messages.
- Messages sent straight to a window don't exercise real mouse input (cursor shape, mouse capture), so those need a
  try by hand.

## Protect the user's state

- The app writes `HKCU\Software\WestieLib\Window` every time it closes. Any script that starts it must set the user's
  values aside first and put them back in a `finally` block, as `ui-checks.ps1` does. Never delete that key without
  restoring it. When the app saves a new value, add its name to `$StateNames`, or it won't be put back.
- The app's files (`settings.json`, `state.json`, `tools\`) go to the folder `WESTIELIB_DATA_DIR` names, when it's set.
  `ui-checks.ps1` sets it to a new folder under `%TEMP%` for the apps it starts, and deletes that folder in its
  `finally` block, so `%LOCALAPPDATA%\WestieLib` is never touched. Any other script that starts the app does the same.
  The checks only type library folders (`C:\WestieLib-check-<random>`, …), and never create them.
- The checks never reach the network:
  - They set `WESTIELIB_GITHUB_API` to `http://127.0.0.1:9`, where nothing listens, so every release look-up fails
    within a couple of seconds with `Couldn't reach 127.0.0.1: …` (in Windows' language). `Start-App` waits for the
    app's first tool run to end (`Wait-ToolsIdle`: Check for updates is enabled again), unless it gets `-DuringToolRun`.
  - `FakeWeb` (embedded C#) is a small HTTP server on 127.0.0.1, which `WESTIELIB_SPOTIFY_URL` points at. It answers
    Spotify's embed page with a name for the ids in `SpotifyNames`, late for `SlowIds`, with HTTP 500 for `BrokenIds`,
    and with Spotify's error page for any other id. Given a `YtDlp` to serve, it also answers yt-dlp's latest release
    on GitHub's API (other repositories get 404), and holds the download back until `AssetGate` is set.
  - The fake yt-dlp is a C# console program the script compiles (`Add-Type -OutputType ConsoleApplication`). It answers
    a playlist look-up by its list id (`$FakeYtDlpSource` lists them) and adds each run's arguments to
    `yt-dlp-runs.txt` beside it. `Install-FakeTools` installs it with empty ffmpeg files.
- The Add playlist look-ups start 300 ms after the link last changed, so read the dialog's line with `Wait-LinkLine`,
  which waits while it says `Looking up…`. `Get-LinkLine` reads it at once, with the Add button's text and state.
- Runs 1 to 5 have no playlists, so they see the welcome screen. Run 6 adds playlists (`Add-Playlist`), run 7 starts
  with them, run 8 starts with more than fit (`Write-ManyPlaylists`) and scrolls them, and run 9 breaks `state.json`;
  then it is deleted, so the later runs see the welcome screen again. Run 10 serves yt-dlp from `FakeWeb` and checks
  that Add playlist moves on from waiting once it is installed.
- A splitter has the mouse capture during a drag, so real mouse moves reach it too. Windows also sends one when a
  window moves under the cursor, which every drag step does when the app lies under the cursor. So a run that drags
  sets the window to bounds of its own first, rather than keeping it maximized. It then waits for the restore
  animation, which a blink check would otherwise count as blinking.
- Run 1 starts without tools, so yt-dlp's install fails, and YouTube Music links wait for it in Add playlist. Run 2
  starts with fake ones (`Install-FakeTools`: a `tools.json`, the fake yt-dlp and empty ffmpeg files) and
  `autoUpdateTools` off, so no release is looked up until Check for updates; it also looks up YouTube Music links.
- Downloads themselves aren't in the checks. To try them by hand, point `WESTIELIB_DATA_DIR` at a scratch folder and
  start the app against the real GitHub, or against a local server that answers `/repos/yt-dlp/yt-dlp/releases/latest`
  with a release whose asset URL is on the same server; sending the asset slowly shows the progress.
- Never cut such a script's output short (`| Select-Object -First n`): PowerShell then stops the script before its
  `finally` block, so the app keeps running and the user's values stay set aside. Filter with `Where-Object` or
  `Select-String`, which read to the end.
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
- Every panel is a `WestieLib.Panel`, and the log's splitter a `WestieLib.Splitter` like the others. `EnumChildWindows`
  also returns hidden windows and the panels inside panels. So the helpers take the main window's own panels
  (`Get-MainPanels`), find one by its name, which is its window text (`Get-Panel $h 'Log'`), find the visible panel row
  by its top edge (`Get-RowPanels`), and tell splitters apart by shape (`Get-ColumnSplitters`, `Get-LogSplitter`).
- An edit control keeps its text itself, so read and write it with `WM_GETTEXT` / `WM_SETTEXT` (`EditText`,
  `SetEditText`), which Windows marshals across processes. `WM_SETTEXT` makes it notify `EN_CHANGE`, as typing does,
  and sending `WM_KILLFOCUS` makes it notify `EN_KILLFOCUS`, as when the user leaves the field.
- `Focus` gives a window the real keyboard focus, through `AttachThreadInput` like `PostCtrlKey`. A click that follows
  then moves the focus for real, as a mouse click would.
- `RedInk` tells whether a window's text is drawn in the error color, in a `PrintWindow` capture.
- A dialog that opens from a button click is modal, so post `BM_CLICK` rather than send it, then wait for the window
  it owns (`Wait-OwnedWindow`).
- To drive the menu bar, post `WM_SYSCOMMAND` `SC_KEYMENU` (with a mnemonic in `lparam` to open that menu) and then
  `WM_KEYDOWN` keys; the app's menu loop picks them up. End with enough Esc presses to leave the menu loop, or the
  next checks run against a window stuck in menu mode (`Check-MenuBarEdge -Menus`).
- `PostCtrlKey` presses a Ctrl shortcut without real input, so nothing can reach the user's other windows: it attaches
  to the app's input thread, sets Ctrl in the shared key state, and posts the key's `WM_KEYDOWN`. The app's
  `TranslateAccelerator` sees Ctrl held, as with a real key press.
- The app sets menu item states when a popup opens, so send `WM_INITMENUPOPUP` before reading them
  (`Update-MenuStates`). `Send-Command` looks a command up by its menu text, so the scripts don't repeat IDs.
- Common controls don't marshal pointers across processes. A message that fills a buffer (`TB_GETBUTTON`,
  `SB_GETTEXTW`, …) needs the buffer in the app's process: use `SendForBuffer`. A message that takes a struct pointing
  at a text buffer (`TTM_GETTEXTW`, `LVM_GETITEMTEXTW`) needs both there: `SendForText` writes the struct into the
  app's process. `TooltipText` and `ListViewRow` build on it.
- A tooltip's owner is the top-level window, not the window its tools are in, so `TooltipText` asks each of the
  process's tooltip windows for the tool.
- `StatusIconColor` tells which icon a status bar part draws, by its color in a `PrintWindow` capture.
  `ChildPixels` returns a child's first columns in such a capture, so two calls a moment apart show whether an icon
  moves (the Add playlist spinner). It stands still when `ClientAreaAnimation` says Windows' animations are off.
- Text the app draws itself (the owner-drawn status bar parts) can't be read back with messages. The checks find it
  in a `PrintWindow` capture instead, as the columns darker than mid-gray (`StatusTextGaps`). `StatusSeparators` finds
  the bar's separator lines the same way. `Check-That` is for checks that pass within a range.
- A `Label`, the All tracks row and buttons keep their text as the window text, which `GetWindowTextW` (`TextOf`) reads
  from another process without a message. So does the Add playlist dialog's detection line, which paints itself
  (`Get-LinkLine`). The playlist rows have no text of their own, so the checks place them by the Add playlist button
  under them (`Check-PlaylistRows`), and the screenshots show them.
- Blinking during a splitter drag only shows on screen: `PrintWindow` paints the window afresh, so it never catches a
  window between an erase and a paint. `WatchDrag` drags a splitter while a second thread keeps copying an area of
  the screen (`CopyFromScreen`, which returns what DWM composed), and counts the captures that blinked. A steady area
  blinks when any pixel differs from the capture before the drag, and moving text blinks when it loses a tenth of its
  dark pixels. It says `covered` when another window lies over the area, because the screen then shows that window instead.
- Keep the scripts ASCII. Windows PowerShell reads a `.ps1` without a BOM as ANSI, so a `…` in it turns into
  mojibake. In embedded C#, write such characters as `(char)0x2026`.
