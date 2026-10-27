# Testing

## `tests/ui/ui-checks.ps1`

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/ui/ui-checks.ps1 -Exe $build\WestieLib.exe -Suite playlists,painting -Screenshot <png>
```

- Starts the exe several times per suite and drives it with Win32 calls through P/Invoke (`Add-Type` C#). The check
  names in the output say what each one covers.
- `-Suite` takes suite names, separated by commas, and runs those in that order. Without it, every suite runs.
- While working on a change, run the suites it touches (see the table below). At the end of a phase, run them all.
- Prints `ok` or `FAIL` per check, then the failures per suite. The exit code is the number of failures. A suite that
  stops on an error counts as one failure, and the next suite still runs.
- `-Screenshot` saves the window to a PNG via `PrintWindow(PW_RENDERFULLCONTENT)`, cropped to
  `DWMWA_EXTENDED_FRAME_BOUNDS`. Read the PNG to look at it.
- Expected sizes are written in DIPs and scaled with the window's DPI (the `S` helper).
- When a monitor with another DPI is attached, the `dpi` suite covers it. Its run 1 moves the window there, which
  sends `WM_DPICHANGED`, and run 2 restores it there from the saved state. Both check everything that scales
  (`Check-Scaled`).
- Usually every monitor runs at the same scale, and that suite prints `Skipped`. After a change to DPI handling (fonts,
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

### Suites

| Suite | Checks | Run it after changes to |
|-------|--------|-------------------------|
| `window` | DPI awareness, icons, menus and their states, About, the toolbar and status bar, the side panels' first-run content, sizes, splitter limits, the log pane, saving and restoring the window state | `ui/main/`, `ui/commands/`, `ui/toolbar/`, `ui/statusbar/`, `ui/log/`, `ui/welcome/WelcomePanel`, menus, accelerators |
| `settings` | The Settings dialog, the library folder field, Browse and Change..., settings.json | `core/settings/`, `ui/dialogs/SettingsDialog`, `ui/dialogs/FolderPicker`, `ui/welcome/LibraryFolder*` |
| `tools` | The Tools part, the Settings dialog's Tools group, Check for updates, installing yt-dlp | `core/tools/`, `core/github/`, `ui/tools/`, `ui/dialogs/ToolListView` |
| `add-playlist` | The Add playlist dialog: opening it, reading links, the Spotify and YouTube Music look-ups, Ctrl+Backspace in the link field | `ui/dialogs/AddPlaylistDialog`, `ui/dialogs/Link*`, `ui/dialogs/PlaylistLinkCheck`, `core/sources/`, `core/playlists/PlaylistLink`, `PlaylistLookup` |
| `playlists` | Adding, keeping, selecting, showing, removing and scrolling playlists, the cards, the playlist menu, state.json | `ui/playlists/`, `ui/inspector/`, `ui/dialogs/RemovePlaylistDialog`, `core/playlists/`, `core/state/` |
| `tracks` | The track panel before it holds tracks: the filter bar, the status tabs, the result line, Clear, the search field's keys (Ctrl+Backspace, Ctrl+Delete, undo off, Enter, Esc, Ctrl+F), the table's columns, sorting and row height | `ui/tracks/`, `ui/framework/TabStrip`, `ui/framework/ListView`, `ui/framework/ComboBox`, `ui/framework/IconView`, `ui/framework/Edit`, `ui/framework/EditKeys`, `ui/framework/WordBreaks` |
| `sync` | Sync all, Sync selected and Sync now: what a YouTube Music or Spotify fetch writes to state.json, the table's rows, the tab counts and result line, the cards; a failed fetch, Cancel, removing a playlist with its tracks, the rows after a restart, sorting them, the Source filter, the search (words in any field and order, case and accents aside) and the tabs, the selection through all that, the track card of a selected row and its playlist link, the cards of tracks a seeded state.json gives a video, a match or a failure (and that saving keeps those fields), matching: the queries, the best result over the first, a live version passed over, no results, a failed search, only another song of the same artist and length, a match that merges into a YouTube Music track and a YouTube Music source that joins a matched track (one row in All tracks, naming both playlists), a playlist that lists one track twice (its second entry a duplicate, with its card, Status tooltip and the Duplicates tab, which only playlists have), a second sync that searches only the failed tracks, and Cancel during a search, the selection card of two, the rows' counts line, their sync controls and tooltips, the playlist card's rows, the infobar (Details, its close box, a failure that comes back), a fetch that waits for yt-dlp, a sync that keeps the table where it is scrolled (`Get-TopRow`, `LVM_GETTOPINDEX`), and a Spotify fetch that shows its pages as they come (also when cancelled between them, or when the list changes), asks for a page again after HTTP 500, or fails | `core/sync/`, `core/state/`, `core/matching/`, `core/sources/YouTubeMusicPlaylist`, `core/sources/Spotify*`, `platform/win32/WinHttpClient`, `ui/sync/`, `ui/tracks/`, `ui/inspector/Track*`, `ui/inspector/SourceLink`, `ui/inspector/CoverArt`, `ui/infobar/`, `ui/playlists/LastSyncLine`, `ui/playlists/PlaylistRowPainter`, `ui/TrackCounts` |
| `painting` | Unpainted areas and blinking during splitter drags | anything that paints a panel or moves panels: `BufferedPaint`, `DeferredPositions`, splitters, new panels |
| `dpi` | Everything that scales, on a monitor with another DPI | DPI handling; ask the user for a monitor at another scale first |

- A change in `ui/framework/` or `platform/` can reach every suite: run them all.
- `ui-checks.ps1` is the runner. It loads `lib/Checks.ps1` once, because `Add-Type` loads a type only once per
  process. That file sets up the app's surroundings and holds the helpers more than one suite uses; the C# lives in
  `lib/W.cs` (the Win32 calls), `lib/FakeWeb.cs` and `lib/FakeYtDlp.cs`. A suite, `suites/<name>.ps1`, has its own
  helpers, then its runs. Move a helper to `lib/Checks.ps1` when a second suite needs it.
- Each suite must stand on its own. Before it, `Reset-Checks` stops a leftover app, deletes the saved window state,
  and empties the data folder, so a suite seeds what it starts from: `Set-SavedWindow` (the registry values),
  `Set-SavedAutoUpdate` (settings.json), `Install-FakeTools`, `Write-ManyPlaylists` (state.json). Never rely on another
  suite having run. Within a suite, a run may build on the one before (run 2 restores what run 1 saved).

## Protect the user's state

- The app writes `HKCU\Software\WestieLib\Window` every time it closes. Any script that starts it must set the user's
  values aside first and put them back in a `finally` block, as `ui-checks.ps1` does. Never delete that key without
  restoring it. When the app saves a new value, add its name to `$StateNames`, or it won't be put back, and to
  `Set-SavedWindow`'s defaults.
- The app's files (`settings.json`, `state.json`, `tools\`) go to the folder `WESTIELIB_DATA_DIR` names, when it's set.
  `ui-checks.ps1` sets it to a new folder under `%TEMP%` for the apps it starts, and deletes that folder in its
  `finally` block, so `%LOCALAPPDATA%\WestieLib` is never touched. Any other script that starts the app does the same.
  The checks only type library folders (`C:\WestieLib-check-<random>`, …), and never create them.
- The checks never reach the network:
  - They set `WESTIELIB_GITHUB_API` to `http://127.0.0.1:9`, where nothing listens, so every release look-up fails
    within a couple of seconds with `Couldn't reach 127.0.0.1: …` (in Windows' language). `Start-App` waits for the
    app's first tool run to end (`Wait-ToolsIdle`: Check for updates is enabled again), unless it gets `-DuringToolRun`.
  - `FakeWeb` (`lib/FakeWeb.cs`) is a small HTTP server on 127.0.0.1, which `WESTIELIB_SPOTIFY_URL` and
    `WESTIELIB_SPOTIFY_API` point at. It answers Spotify's embed page with a name and an access token
    (`fake-token-<id>`) for the ids in `SpotifyNames`, late for `SlowIds`, with HTTP 500 for `BrokenIds`, and with
    Spotify's error page for any other id. It answers the `fetchPlaylist` query like Spotify: HTTP 400 without
    `enableWatchFeedEntrypoint`, 401 without the playlist's token, `PersistedQueryNotFound` for `ErrorQueryIds`, the
    page of `SpotifyItems` that `offset` and `limit` ask for, and `NotFound` for ids without items. Pages after the
    first wait while `SpotifyPageGate` is reset, so the `sync` suite can look at the app between pages and cancel there.
    `SpotifyFailures` makes the query for `"<playlist id> <offset>"` answer HTTP 500 that many more times. `Track`, `Episode`
    and `LocalFile` build those items, with fields the app doesn't read too, and `SpotifyQueries()` lists the queries
    it got with their headers. Given a `YtDlp` to serve, it also answers yt-dlp's latest release on GitHub's API (other
    repositories get 404), and holds the download back until `AssetGate` is set.
  - `WESTIELIB_YTMUSIC_API` points at `FakeWeb` too, which answers YouTube Music's search
    (`POST /youtubei/v1/search`) in its shape, and HTTP 400 without the songs filter. A query gets the songs
    `YtmResults` lists for it (built with `Song`; an empty list for no results), HTTP 500 when it is in
    `YtmBrokenQueries`, or else, when it is `"<artists> <title>"` of an item `Track` built, that song as video
    `yt<track id>`; anything else has no results. So a check that only fetches Spotify playlists sees their tracks
    matched and Waiting. `YtmQueries()` lists the searches, and each waits for `YtmGate`, which the `sync` suite
    resets to look at the app while it matches and to cancel there.
  - The fake yt-dlp is a C# console program the script compiles (`lib/FakeYtDlp.cs`, with
    `Add-Type -OutputType ConsoleApplication`). It answers a playlist look-up (`--playlist-items 0`) and a fetch by
    the list id (the file lists them): songs for `PLtracks…` and `PLshared…`, which share one, with a ` - Topic`
    channel, a private video, a repeat and a missing length among them. It adds each run's arguments to
    `yt-dlp-runs.txt` beside it. `Install-FakeTools` installs it with empty ffmpeg files.
- The Add playlist look-ups start 300 ms after the link last changed, so read the dialog's line with `Wait-LinkLine`,
  which waits while it says `Looking up…`. `Get-LinkLine` reads it at once, with the Add button's text and state.
- `Add-Playlist` turns `Sync now` off, unless it gets `-SyncNow`: a sync saves state.json again, which a check of the
  file or its backup would race.
- A sync enables Cancel until it ends (`Test-Syncing`). The `sync` suite waits for one with `Wait-Fetches`: first until
  the fake yt-dlp has logged the fetches it expects, which comes after the window knows of the sync, then until Cancel
  grays; with `-Spotify` it counts FakeWeb's Spotify queries instead. `Get-TrackRows` reads the table's rows through `ListViewRow`, and `Get-SavedPlaylistSync`,
  `Get-SavedSources` and `Get-SavedTracks` read state.json.
- `Select-TrackRow` selects a table row with Home and Down (`WM_KEYDOWN`), not a click: the list checks a sent click
  against the real cursor, which is elsewhere, starts a drag, and the drag then takes the next click on a header.
  `Get-SelectedRows` lists the selected rows (`LVM_GETNEXTITEM`). `Choose-Source`, `Click-Tab` and `Click-Header`
  are in `lib/Checks.ps1`, since both track suites use them.
- A playlist row with tracks waiting, failed or to review is a line taller. The rows paint themselves, so the script
  can't measure them: a suite that knows which rows are tall lists them in `$script:TallRows` (from 1), which
  `Get-PlaylistRowTop` and `Click-PlaylistRow` add up. `Reset-Checks` empties it.
- While the infobar shows, the panels start under it, and `Get-RowTop` (which `Get-RowPanels` uses) starts there too.
  `Get-Infobar` finds it; its close box is painted, so the `sync` suite clicks it by its place (24 DIPs, 6 from the
  right edge).
- The track table's Status cell has a tooltip tool (id 1 on the list) for the cell the mouse last moved onto. The
  `sync` suite's `Get-StatusTip` sends `WM_MOUSEMOVE` to the middle of a row's Status cell and reads the tool's text.
- A row's sync control is painted too, but it has a tooltip tool: `ToolRect` reads the tool's rect
  (`TTM_GETTOOLINFOW`), which is where to click, and `TooltipText` its text.
- `Get-CardTexts` reads only the card's visible windows, since the rows a card hides are still its children.
- The status bar's Run part is owner-drawn, so only the screenshots show it (`-syncing.png` while a fetch runs).
- Suites start without playlists, so they see the welcome screen. The `playlists` suite adds some in run 1
  (`Add-Playlist`), starts with them in run 2, starts with more than fit in run 3 (`Write-ManyPlaylists`) and scrolls
  them, and breaks `state.json` in run 4. The `tools` suite's run 3 serves yt-dlp from `FakeWeb` and checks that Add
  playlist moves on from waiting once it is installed.
- A splitter has the mouse capture during a drag, so real mouse moves reach it too, among them the one Windows sends
  when a window moves under a cursor that stands still. The splitter only follows moves with the left button held,
  which the checks send (`MK_LBUTTON` in `wparam`) and those moves don't. Before that, the drags failed whenever
  another app than the one that started the script was in front. A run that drags still sets the window to bounds of
  its own first, and never starts maximized: the restore animation would count as blinking.
- Suites start without tools, so yt-dlp's install fails, and YouTube Music links wait for it in Add playlist. A run
  that needs tools installs fake ones first (`Install-FakeTools`: a `tools.json`, the fake yt-dlp and empty ffmpeg
  files), mostly with `autoUpdateTools` off (`Set-SavedAutoUpdate $false`), so no release is looked up until Check for
  updates.
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
- Ctrl+Backspace reaches an edit as `WM_CHAR` `0x7F`, and Enter and Esc as `0x0D` and `0x1B`. Send those rather than
  post them: a message sent from another thread is handled before the ones posted earlier, so a `WM_GETTEXT` sent
  right after a posted key can read the text before the key changed it. Ctrl+Delete is a `WM_KEYDOWN` that needs Ctrl
  held, so it goes through `PostCtrlKey`, which waits for it. `EM_SETSEL` places the caret first.
- `Focus` gives a window the real keyboard focus, through `AttachThreadInput` like `PostCtrlKey`. A click that follows
  then moves the focus for real, as a mouse click would.
- `RedInk` tells whether a window's text is drawn in the error color, in a `PrintWindow` capture.
- `EditTextTop` gives where an edit's line of text starts in its window (its client area's top plus `EM_GETRECT`'s),
  and `TextHeight` the message font's `tmHeight`, so the `tracks` suite checks that the search text is centered.
- A dialog that opens from a button click is modal, so post `BM_CLICK` rather than send it, then wait for the window
  it owns (`Wait-OwnedWindow`).
- To drive the menu bar, post `WM_SYSCOMMAND` `SC_KEYMENU` (with a mnemonic in `lparam` to open that menu) and then
  `WM_KEYDOWN` keys; the app's menu loop picks them up. End with enough Esc presses to leave the menu loop, or the
  next checks run against a window stuck in menu mode (`Check-MenuBarEdge -Menus`).
- `PostCtrlKey` presses a Ctrl shortcut without real input, so nothing can reach the user's other windows: it attaches
  to the app's input thread, sets Ctrl in the shared key state, and posts the key's `WM_KEYDOWN`. The app's
  `TranslateAccelerator` sees Ctrl held, as with a real key press. `PostShiftKey` holds Shift the same way, which the
  `sync` suite uses to select a second row with Shift+Down.
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
- The selection shows only as paint. `Get-Selection` samples a pixel just inside the top-left corner of All tracks and
  of each row in view (`PixelsIn`, from one `PrintWindow` capture), and lists the ones in the selection color.
  `Get-PlaylistRowTop` places a row from its height, the gaps and the list's `GetScrollPos`. `Get-PlaylistCommands`
  reads whether the commands that need a selected playlist are enabled. `Get-RowLooks` reads the same pixels as
  `blue` (selected or pressed), `hot` or `-`, and `Check-RowMouse` sends mouse messages one at a time (`Send-Mouse`)
  to walk through press, dragging off and back, and a lost capture (`WM_CANCELMODE`, whose default handling releases
  it). A sent `WM_MOUSEMOVE` can't show a hover: the row asks for `WM_MOUSELEAVE`, which Windows sends at once because
  the real cursor is elsewhere. The checks only confirm that, and a real hover needs a try by hand. So the `playlists`
  suite saves a window position first (`Set-SavedWindow`): at the place Windows picks, the window can land under the
  cursor, and the rows stay hot.
- `Click-PlaylistRow` and `Click-AllTracks` send `WM_LBUTTONDOWN` and `WM_LBUTTONUP` (`Send-Click`), and
  `Press-ListKey` sends `WM_KEYDOWN` to the list, which acts on it whether or not it has the focus. `FocusOf` tells
  which window has the focus on the app's thread (`GetGUIThreadInfo`).
- The inspector shows one card at a time, a panel named `Library card`, `Playlist card`, `Track card`
  or `Selection card`
  (`Get-InspectorCard`), or none on the first run. It follows the table's selection on a posted message, so after
  selecting rows read it with `Wait-CardTexts`, which waits for the card it's given. From one track to another the
  card's name stays, so the `sync` suite's `Wait-TrackCard` waits for the track's key instead; while the inspector
  swaps cards, for a moment no card shows. `Get-CardTexts` lists the card's name and its windows' texts from the top, left to right
  within a row, and `Get-CardLayout` gives where the card, its title, its values and its buttons sit.
- A context menu runs a modal loop, so post `WM_CONTEXTMENU` (`lparam` -1 for the keyboard's), then find the popup as
  the process's visible `#32768` window (`Wait-PopupMenu`). `MN_GETHMENU` gives its menu, which `Get-PopupItems`
  reads. The loop takes keys posted to any window of the app's thread: Esc closes it (`Close-PopupMenu`), an access
  key chooses.
- Windows marshals the messages below `WM_USER` across processes itself, so `CB_GETLBTEXT` takes a local buffer
  (`ComboItems`); a buffer in the app's process would be read as an address in the script's. Common control messages
  (`TCM_*`, `HDM_*`, `LVM_*`, `EM_GETCUEBANNER`) need theirs in the app's process: `TabTexts`, `ListViewColumns`
  (titles, widths and the sort arrow in display order), `CueBanner`, and `SendForRect` for the messages that fill a
  `RECT`.
- `CB_SETCURSEL` and `TCM_SETCURSEL` don't notify. The `tracks` suite chooses a source with `CB_SETCURSEL` and then
  sends the parent `CBN_SELCHANGE`, as a choice by the user would, and clicks tabs and headers with mouse messages at
  `TCM_GETITEMRECT` / `HDM_GETITEMRECT` (`Click-Tab`, `Click-Header`). `Double-Click-Header` sends the down, up,
  `WM_LBUTTONDBLCLK`, up of two quick clicks. The `tracks` suite's table holds no rows, so the row height
  check gives it three with `LVM_SETITEMCOUNT` and measures one.
- A TaskDialog's buttons aren't child windows. Post it `TDM_CLICK_BUTTON` (`WM_USER + 102`) with the button's id.
- The checks never run Open in browser, the playlist card's link, a track card's `Open` links, Open library folder or
  Copy URL: they would open a
  browser or Explorer on the user's screen, or replace what's on their clipboard. Try those by hand.
- Blinking during a splitter drag only shows on screen: `PrintWindow` paints the window afresh, so it never catches a
  window between an erase and a paint. `WatchDrag` drags a splitter while a second thread keeps copying an area of
  the screen (`CopyFromScreen`, which returns what DWM composed), and counts the captures that blinked. A steady area
  blinks when any pixel differs from the capture before the drag, and moving text blinks when it loses a tenth of its
  dark pixels. It says `covered` when another window lies over the area, because the screen then shows that window instead.
- Keep the scripts ASCII. Windows PowerShell reads a `.ps1` without a BOM as ANSI, so a `…` in it turns into
  mojibake. In the `.cs` files, write such characters as `(char)0x2026`.
