# Main window

`MainWindow` (`src/ui/main/`) is the frame window. It owns the menu bar (`MainMenu.cpp`), the window icon, every
child window as a member (declaration order is creation order), its `WindowPlacement`, and the user's `PanelState`
(panel sizes in DIPs, and whether the log shows). It lays the children out and routes commands. What the window
should contain and how it should behave is specified in `plan/ui.md`; this file covers how the code is put together.

## Commands

- Every menu item, toolbar button and shortcut is a `CommandId` (`src/ui/commands/`). The values double as Win32
  command IDs.
- `CommandState` holds what the commands depend on (a sync running, whether the tools are at work, whether there are
  playlists, the selection, whether the log shows). `has_playlists` stays false until playlists exist.
  `enabled(id)` and `checked(id)` are the only enable rules, so the menu, the toolbar and the shortcuts can't
  disagree. `MainWindow::command_state()` builds it from the window's current state.
- `MainMenu.cpp` lists the menus as tables of `command_item(id, text)`, with the shortcut after a tab. On
  `WM_INITMENUPOPUP` (not for the window menu) `update_menu_items` grays and checks the popup's items from
  `command_state()`.
- `MainAccelerators.cpp` holds the shortcut table. `main.cpp` builds it and hands it to `MessageLoop::run`. Esc and
  Enter appear in the menu text but aren't in the table: they belong to the sync and the track list.
- `MainWindow::handle_message` first offers every message to its `MenuBarPainter`, which draws the menu bar in white
  so it runs into the toolbar.
- Menus and accelerators arrive as `WM_COMMAND` with no control. `run_command` drops a command that
  `command_state()` says is disabled, then acts on it. Commands without a feature yet do nothing.

## Startup and shutdown

`WindowPlacement` (`WindowPlacement.cpp`) handles the window's bounds and maximized state; `MainWindow` hands it the
saved `Placement` and asks it for the current one.

1. Before the window, `main.cpp` makes the `SettingsStore` (`src/core/settings/`) with the defaults and loads the `Settings`;
   a `settings.json` that doesn't read ends the app there. Then it makes the `StateStore` (`src/core/state/`), which
   loads `state.json`; one that doesn't read ends the app too. The files live in `%LOCALAPPDATA%\WestieLib`, or in
   the folder `WESTIELIB_DATA_DIR` names, which the UI checks use. Then it makes the `CoreEventQueue`, the
   `ToolFolder`, the `ToolManager`, `YtDlp`, the `PlaylistLookup` and the `SyncEngine`, so they outlive the window.
   `MainWindow` gets the settings store, the loaded settings, the state store, the queue, the tool manager, the
   look-up and the sync engine, and makes itself the queue's target.
2. The constructor creates the window hidden, creates its children, sets the icons and attaches the menu.
3. If `load_window_state()` finds saved state, the constructor takes the `PanelState` from it and calls
   `WindowPlacement::restore`, which runs `SetWindowPlacement` with `SW_HIDE` and the saved normal bounds. Without
   saved state, `apply_initial_size` uses `InitialWindowDips`, clamped to the monitor's work area.
4. `main.cpp` starts a tool run (`ToolManager::run`), then calls `show(nCmdShow)`. Posted messages go before
   `WM_PAINT`, so the first paint already shows the run. `WindowPlacement::show_command` turns `SW_SHOWNORMAL` /
   `SW_SHOWDEFAULT` into `SW_SHOWMAXIMIZED` when the saved window was maximized; any other show command, say from a
   shortcut, wins.
5. `WM_DESTROY` → `ToolManager::stop()`, `PlaylistLookup::stop()`, `SyncEngine::cancel()` and
   `CoreEventQueue::set_target(nullptr)` →
   `save_state()` → `WelcomePanel::commit_library_folder()` → `PostQuitMessage(0)`. The workers wind down while the
   window goes, and `main.cpp` joins them after the window is gone; what they still report stays in the queue. The
   commit is for a folder still being typed, since a close doesn't take the focus from the field first.
   `WindowPlacement::current` reads `GetWindowPlacement` and also counts a window minimized with
   `WPF_RESTORETOMAXIMIZED` as maximized. A window torn down by an exception saves nothing, because `~Window` detaches
   before destroying.

## Toolbar and status bar

- `MainToolbar` (`src/ui/toolbar/`) is a strip window. It paints the white background and the bottom rule, and hosts
  a `Toolbar` control. A button click, or a choice from the Sync all dropdown, goes to the callback `MainWindow`
  gives it, which is `run_command`. `update(CommandState)` enables the buttons and keeps the state for the dropdown.
- `MainStatusBar` (`src/ui/statusbar/`) owns the `StatusBar`, its part widths (the `*WidthDips` shares and minimums,
  from `plan/ui.md`), its spacing, and the part texts:
  Run, Tracks and Library from the left, an empty part, then Tools and Version against the right edge, with Version's
  text centered. Until the core exists, Run reads `Ready` and Version shows the build's version. Library shows the
  folder in effect, with a path ellipsis, and is the part that gives way when Tools needs the room. The texts are
  owner-drawn, so `MainWindow` hands the status bar's `WM_DRAWITEM` to `MainStatusBar::draw`.
- `MainStatusBar::set_tools` takes the `ToolBoard`. The Tools part reads `Tools` and an icon: the board's `ToolMark`
  icon (✓ or ⚠) while idle. While a run goes, `tools_work` gives it the progress ring, filled to the work's
  percentage, and what is going on after the ring (`installing ffmpeg 35%`). The tooltip lists the releases either
  way. It starts at Working, since `main.cpp` starts a run right away.
- The Tools part is as wide as the widest of `every_tools_work_text`, measured in the bar's font with the percentage
  at `100%` (`measure_tools`, again after each new font), so it never changes width. The ring's 25 steps are icon
  resources from `IDI_RING_0`, loaded at 12 DIPs.

## Core events and tools

- `CoreEventQueue` (`src/ui/events/`) is the core's `EventSink`. Any thread can `emit`; when the queue was empty it
  posts `CoreEventsMessage` to the window, and `MainWindow::on_core_events` takes every queued event and `std::visit`s
  each one onto an `apply_event` overload. Then `show_tools` updates the status bar, the commands and an open
  Settings or Add playlist dialog.
- `ToolBoard` (`src/ui/tools/`) is the UI's view of the tools: a `ToolStatus` per download, whether a run is going
  (`ToolsRunChanged`) and when the releases were last checked (`ToolsChecked`). Commands don't depend on it, except
  `Check for updates`, which waits for a run to end: work that needs a missing tool waits for it in the sync instead
  (`plan/arch.md`, ToolManager). `ToolMark` turns a status, or the board, into Ready, Working or Attention.
  `available(package)` says whether a tool can run: its installed release is known, which stays so through a failed or
  running update.
  `ToolStatusText.cpp` has all the wording, and `RelativeDayText.cpp` the `today 14:03` / `yesterday` / `12 Oct`
  style the playlist rows will share.
- `Check for updates` calls `ToolManager::run(true)`. The run itself reports that it started, so the command grays
  until it reports that it ended.
- `show_settings` keeps the dialog in `settings_dialog_` while it's open, so tool events reach its list. It saves the
  settings when anything in them changed.
- **Sync.** `SyncBoard` (`src/ui/sync/`) is the UI's view of the `SyncEngine`'s run, as `ToolBoard` is of the tools:
  whether it goes (from `SyncQueued` until `SyncFinished`), the playlists queued, the one being fetched and the tools
  it waits for. `CommandState::syncing` comes from it.
  - `Sync all` hands the engine every playlist in row order, `Sync selected playlist` (also the menu's Sync and the
    card's button) the selected one, and Add playlist's `Sync now` the new one. The engine leaves Spotify playlists
    out. `Cancel` calls `SyncEngine::cancel`.
  - `PlaylistFetched` and `PlaylistFetchFailed` bring the window up to date from a new snapshot (`show_playlists`).
  - Esc for Cancel isn't in the accelerator table, because the table would take Esc from every control, such as an
    open drop-down list.
- **State.** The sync's worker writes the state, so the window reads it through `StateStore::snapshot()`, a
  `std::shared_ptr<const State>` that stays as it is while the window uses it. Every function that shows the state
  takes one snapshot and works from it.
- Both measure their height in `apply_dpi`. `MainWindow::apply_dpi` makes the `FontSet` and hands the regular font
  and the DPI to both, and the set to the side panels. It runs in the constructor, on `WM_DPICHANGED`, and on
  `WM_SETTINGCHANGE` with `SPI_SETNONCLIENTMETRICS` (a new system font size).

## Side panels

- `PlaylistPanel` (`src/ui/playlists/`) stacks the `AllTracksRow`, a rule it paints itself, and the `PlaylistList`
  under it, which fills the rest of the panel. Only the list scrolls.
- `PlaylistList` is a window that paints the playlist rows and holds the `No playlists yet.` label (only while there
  are no rows) and the `Add playlist` button after the last row. It scrolls itself: `layout` works out the content's
  height, sets the scroll bar (`SetScrollInfo`; showing or hiding the bar sends `WM_SIZE`, which lays out again), and
  places the label and the button at their offset less the scroll position. A `framework::VerticalScroll` keeps the
  position and turns `WM_VSCROLL` and `WM_MOUSEWHEEL` into it. The button's click goes to `PlaylistEvents::on_command`,
  which `MainWindow` sets to `run_command`, as for the toolbar.
- **Selection.** `MainWindow::selected_playlist_` is the one copy of the selection: a playlist's key, or nothing for
  All tracks. The left panel only reports what the user picks and paints what it's told:
  - A click on a row or a key (`WM_KEYDOWN`: Up, Down, Home, End, PgUp, PgDn, with All tracks as the item before the
    first row) calls `PlaylistEvents::on_select`. A click is a press and a release over the same row; the list takes
    the focus on the press. A click on `AllTracksRow` selects nothing, and `PlaylistPanel` gives the list the focus, so
    the keys work from there too.
  - Both windows hot-track and press their rows through a `framework::ItemTracker` (the All tracks row is its one
    item, the list's rows are hit-tested against their rects), and paint each row's `RowLook` (`row_look`,
    `paint_row_look` in `src/ui/RowLook.cpp`): hover, pressed or selected. The list also asks where the cursor is after
    it scrolls (`follow_cursor`), because rows that move under a still mouse get no `WM_MOUSEMOVE`.
  - `MainWindow::select_playlist` stores the key, hands it back through `PlaylistPanel::show_selection`, which tells the
    All tracks row whether it's selected and the list which key is, and updates the commands (`playlist_selected`).
    The list keeps the key, not an index, and scrolls the selected row into view with its gaps (`VerticalScroll::reveal`).
  - `show_playlists` drops a selection whose playlist is gone, and `add_playlist` selects the playlist it added. The
    selection isn't saved, so every start begins on All tracks.
- `MainWindow` keeps a `framework::FocusMemory`: on `WM_ACTIVATE` with `WA_INACTIVE` it notes which child had the
  focus, and on `WM_SETFOCUS`, which `DefWindowProc` sends the window when it's active again, it hands the focus back.
- `PlaylistRowPainter` draws one row into a given rect, with the logo, the waiting and sync glyphs at 14 DIPs and the
  fonts it got, and knows the row's height. The list makes a new one on every `apply_dpi`. The names, the label and
  the button all start at `text_left()`, which adds up the same DIPs as `AllTracksRow::text_left()`, so everything
  lines up with All tracks at every scale. Rows have a fixed height until their counts can be non-zero.
- `playlist_rows` (`PlaylistRows.cpp`) turns the state into `PlaylistRow`s, in the order the playlists were added,
  with `YouTube Music playlist` / `Spotify playlist` for a playlist without a name, and each one's `TrackCounts`. `MainWindow::show_playlists`
  hands them to the panel.
- `AllTracksRow` is a window that paints the row: its look (hover, pressed or selected), glyph, name in semibold, and
  the counts.
- `InspectorPanel` (`src/ui/inspector/`) shows a card at the top of its padding, or nothing on the first run.
  `MainWindow::show_inspector` picks from the state and the selection: no playlists, a selected playlist
  (`playlist_row`), or else All tracks. It runs whenever one of those changes, and when
  the library folder or the track counts (`track_counts_`) change. `update_commands` hands the inspector the
  `CommandState`, so the cards' buttons follow the same rules as the menus.
  - `LibraryCard` and `PlaylistCard` are panels of `Label`s, `Button`s and small windows that paint themselves:
    `CardTitle` (glyph and name in the title font), `SectionTitle` (semibold text and the rule after it),
    `SyncedValue` (`paint_sync_bar`, from `src/ui/SyncBar.cpp`, which the playlist rows use too, then the counts), and
    a `framework::LinkLabel`. Every window's text is what it shows, so the checks read the cards with
    `GetWindowTextW`.
  - `CardLayout` holds the measures from `plan/ui.md` (Inspector › Nothing selected) and stacks a card's parts from
    the top: title, subtitle, section, key–value rows and a row of buttons that wraps. A card runs the same `arrange`
    to place its children on `WM_SIZE` and, without `DeferredPositions`, to measure `height_for(width)`.
  - The panel is `WS_VSCROLL` with a `framework::VerticalScroll`. Its content is the card's height plus the padding;
    scrolling moves the card window, not its children. A new card, or new contents, starts at the top.
  - The cards' buttons and the link send `CommandId`s (the link `OpenPlaylistPage`, which isn't in the menu bar) to
    `run_command`, like the menus.

## Playlist actions

- **Context menu.** `PlaylistList` handles `WM_CONTEXTMENU`. A right-click hit-tests the rows, selects the one it hit
  (`on_select`) and takes the focus; the keyboard's (-1, -1) uses the selected row's bottom-left corner, and does
  nothing for All tracks. Then `PlaylistEvents::on_context_menu` hands the screen point to
  `MainWindow::show_playlist_menu`, which shows `choose_playlist_command` (`PlaylistMenu.cpp`): a popup built from a
  `command_item` table, its states set by `update_menu_items` from `command_state()`, tracked with `TPM_RETURNCMD`
  (and `TPM_NONOTIFY`, since it sets the states itself). The chosen command goes to `run_command`. `CopyPlaylistLink`
  and `OpenPlaylistPage` aren't in the menu bar; like the others that need a playlist, `playlist_selected` enables
  them.
- **The commands** act on the selected playlist (`selected_row`, which is `playlist_row` for the selection):
  - `RemovePlaylist` asks `confirm_remove_playlist` (`RemovePlaylistDialog.cpp`, a TaskDialog with `Remove` as `IDOK`
    and Cancel the default), then `core::remove_playlist` (`src/core/playlists/RemovePlaylist.cpp`) erases it through
    `StateStore::update`, the selection goes back to All tracks and `show_playlists` brings the window up to date,
    welcome screen and all after the last one.
  - `OpenPlaylistPage` and `OpenLibraryFolder` go through `platform::win32::open_with_shell` (`ShellExecuteExW`,
    `open`). Without `SEE_MASK_FLAG_NO_UI` the shell reports its own failures, so the result only says whether it
    worked. The library folder may not exist before the first sync, so `platform::nearest_existing_folder` (which
    the folder picker shares) gives the folder or the nearest one above it.
  - `CopyPlaylistLink` calls `platform::win32::set_clipboard_text` (`ClipboardScope` opens and closes the clipboard,
    `UniqueGlobal` holds the memory until `SetClipboardData` takes it). Another program may hold the clipboard, so a
    failure beeps instead of ending the app.
- `count_tracks` (`src/ui/TrackCounts.cpp`) counts a playlist's tracks, or all of them, by status: a track the
  playlist lists twice, or through two sources, counts once. `MainWindow::show_track_counts` hands the library's to
  the All tracks row and the status bar, and keeps them for the Library card.

## Adding a playlist

- `Add playlist` (toolbar, File menu, Ctrl+N, the left panel's button, the welcome button) runs `add_playlist`, which
  shows `AddPlaylistDialog` (`src/ui/dialogs/`). The dialog gets `added_playlist_name`, which gives the name a link's
  playlist has in the state, or nothing when it isn't there, and the `core::PlaylistLookup` and the `ToolBoard`.
  `MainWindow` keeps the dialog in `add_playlist_dialog_` while it's open, so `PlaylistLookedUp` events
  (`show_lookup`) and tool changes (`show_tools`) reach it.
- On every `EN_CHANGE` the dialog runs `check_playlist_link` (`PlaylistLinkCheck.cpp`): empty, not supported, already
  added (with the stored name), or new, with the parsed `core::PlaylistLink` (`core::parse_playlist_link`,
  `src/core/playlists/`).
- `LinkLookup` (`LinkLookup.cpp`) holds the look-up's state for the dialog's link: none, waiting for tools, looking up,
  found (with the name), unavailable, or failed (with the reason).
  - `set_link` stops the look-up before it and decides at once: an already added playlist with a stored name is found
    without a look-up, a YouTube Music link waits while the board says yt-dlp isn't available, and anything else is
    due. A link to the same playlist keeps the state, unless the look-up failed.
  - A due look-up starts when the dialog's 300 ms `WM_TIMER` fires (`start`); every change restarts the timer.
  - `tools_changed` starts a waiting look-up once yt-dlp is available, and `apply` takes the answer for the current
    link only. A `ToolMissing` answer waits again.
- `link_line` (`LinkLine.cpp`) has the wording: the icon (source logo, ⧗ `IDI_TOOL_WAIT` or ⚠) or the spinner, the
  text, whether it's gray, and the source's name that follows a found name in gray. `add_button_text` says `Add` or
  `Already added`.
- While the line shows the spinner, the dialog runs a `framework::Animation` on a second `WM_TIMER`, every 16 ms, and
  repaints the line at each tick. `paint_spinner` turns the arc by the animation's progress through a one-second turn.
  `playlist_type_text` (`src/ui/PlaylistTypeText.cpp`) gives the source's name and `source_logo_icon`
  (`src/ui/SourceLogo.cpp`) its logo.
- The line under the link is an `SS_OWNERDRAW` static that `paint_link_line` (`LinkLinePainter.cpp`) draws in
  `WM_DRAWITEM`, loading the icon at the static's DPI, or drawing the spinner at that size. It draws into a memory bitmap
  and copies that in one `BitBlt`, so the text doesn't blink as the spinner turns. A found name gives way to the
  source's name, which stays whole.
  The static's window text is the line's text with the source's name after a space, for screen readers and the UI
  checks. `Add` is enabled only for a new playlist that was found.
- On `Add` the dialog returns an `AddPlaylistRequest` with the found name. `core::add_playlist` stores it through
  `StateStore::update`, which saves `state.json` (keeping `state.json.bak`). Closing the dialog stops the look-up.
  With `Sync now`, the new playlist then goes to the `SyncEngine`.
- `show_playlists` then brings the window up to date with the state: the left panel gets the rows, the inspector
  shows the card for the selection, `layout_panels` swaps the welcome screen for the `TrackPanel`, and the commands
  follow (`has_playlists` enables Sync all). The welcome screen may have had the keyboard focus, so it goes to the
  window. The constructor calls it too, for playlists from an earlier start.

## Welcome screen

- `WelcomePanel` (`src/ui/welcome/`) is the main column while there are no playlists; `layout_panels` hides the
  `TrackPanel` meanwhile, and the other way round once there are. It paints the app icon and lays out its labels, the `LibraryFolderField` and the default
  `Add your first playlist` button in one centered column. The button's click goes to `run_command`, as in the side
  panels.
- `LibraryFolderField` is a panel with the `Edit`, `Browse…` and the hint `Label` under them. It keeps the folder in
  effect.
  - Text goes through `check_library_folder` (`LibraryFolderCheck.cpp`). That function cleans the text up with
    `parse_library_folder` (`LibraryFolderText.cpp`, which has the syntax rules), then looks the path up with
    `platform::win32::probe_path`. The result is the folder, if the library can use it, and a `FolderHint`;
    `hint_text` has the wording.
  - `probe_path` never asks a network share or a mapped network drive, because an unreachable server would hang the
    window on every keystroke.
  - On `EN_CHANGE` the field shows the hint, red for a problem.
  - On `EN_KILLFOCUS`, and after `pick_folder` (`src/ui/dialogs/`) returns a folder, a usable folder replaces the text
    in its cleaned-up form and, when it changed, goes to the callback. Anything else leaves the folder in effect alone.
  - The hint label keeps the height of the longest hint (`ShownFolderHints`) at the field's width, so the column
    doesn't move while the user types.
- `MainWindow` hands the welcome panel the default folder (`SettingsStore::defaults()`) for the hint's example, and
  the saved folder (`Settings::library_path`) to start the field and the status bar at. The field's callback,
  `change_library_folder`, saves the settings and updates the status bar.

## Main panel

`TrackPanel` (`src/ui/tracks/`) is the main column once there are playlists. From the top it stacks the `FilterBar`, the `StatusFilter` strip, the result line (a
gray `Label`) and the `TrackTable`, with the paddings from `plan/ui.md` (Main panel), and paints the hairline above
the table itself, through `BufferedPaint`.

- `FilterBar` is a panel with the search `Edit` (its cue banner, and the magnifier, an `IconView` inside the edit),
  the `Source` label (one line tall and centered, so its text stands level with the search field's), the `ComboBox` and
  the compact `Clear` button. The search field narrows when the panel is too
  narrow for everything. `EN_CHANGE` and `CBN_SELCHANGE` call its change callback, and Clear empties the search, sets
  All and calls its clear callback. A clicked Clear grays out, so the focus moves to the search field first.
- `StatusFilter` keeps the selected `StatusTab` and the `StatusCounts`, and shows them on a `framework::TabStrip`:
  `status_tab_text` (`StatusTabText.cpp`) gives each label, and Removed and Kept appear only with a count or while
  they're selected. It maps the strip's index back to a tab on `TCN_SELCHANGE`, which the panel forwards.
- The panel enables Clear while the filter bar or the tab is off its default, and Clear puts the tab back to All.
- `TrackTable` owns the `framework::ListView`. Its column table (`TrackColumn`, title, width, alignment, shown by
  default) is in `TrackTable.cpp`. Title is sub-item 0, because column 0 holds the item's text and can't be
  right-aligned or removed; the column order puts Status before it. `show_scope` inserts `#` and moves it to the front
  of the order, or deletes it, and goes back to the scope's default sort. `LVN_COLUMNCLICK` sorts by the clicked
  column, or turns the order round when it's sorted by that column already, and moves the header's arrow. Source,
  Match and File are in the table but hidden: the header menu that shows them comes later.
- `track_rows` (`TrackRows.cpp`) turns a snapshot into the scope's `TrackRow`s: all tracks, in key order, or the
  playlist's, by where it first lists each. A row has the track's status as a `StatusTab`, the title, artists and
  length of its first source, the names of the playlists that list it (in the order they were added), its position,
  and the kinds of source it has.
- `TrackPanel` keeps the scope's rows and narrows them in `show_rows`: `rows_from_source` by the filter bar's
  `source()`, then `rows_in_tab` by the selected tab (Synced takes Needs review in too). The tabs count the rows
  before the tab narrows them, so each shows what it would list. The source, a tab, Clear and new rows all show the
  rows again. The search field doesn't filter yet.
- `TrackTable` owns the sort: `set_rows` and a header click sort the rows with `sort_track_rows` (`TrackSort.cpp`).
  Text columns compare as Explorer sorts names (`CompareStringEx` with `LINGUISTIC_IGNORECASE |
  SORT_DIGITSASNUMBERS`), # by position, Time by length, Status by the tabs' order; rows that tie go by title, then
  key, whichever way the column sorts. The list's selection is by index, so the table notes the selected and focused
  tracks' keys before the rows change and selects them again after (`ListView::selected_items`, `select_items`). A
  sort also scrolls the focused row into view. Another tab (`show_tab_change`, which Clear goes through too) or
  another scope (`show_tracks`) clears the selection first, so nothing carries over.
- `TrackTable::set_rows` keeps the rows and hands the list their count. The list asks for each cell in
  `LVN_GETDISPINFOW` by the column's index, which `column_subitem` turns into the `TrackColumn`. The Status column has
  no text: `NM_CUSTOMDRAW` draws the status's 16 DIP icon in the cell after the row is painted (`ListView::cell_rect`).
  So far Waiting, Synced and Needs review have icons. Its tooltip comes with the other statuses.
- `MainWindow::show_scope` hands `TrackPanel::show_tracks` the selected playlist's row (or nothing for All tracks),
  the rows and the library's track count, then shows the counts and the inspector. A playlist that isn't the one
  before goes back to the default sort; the same one, with new rows after a fetch, keeps its sort. The status tabs
  count the rows (`status_counts`), and `result_line_text` (`ResultLineText.cpp`) adds the `in …` part.
  `select_playlist` and `show_playlists` call it.

## Layout

- `MainLayout.cpp` is pure: it takes the client rect, the DPI, the bar heights in pixels and the `PanelState`, and
  returns a rect per child, with no window calls. From the top: the toolbar, the infobar row (only when its height
  isn't 0), the panel row, the log splitter and log (only while the log shows); the status bar takes the bottom.
  Size limits are constants in `MainLayout.h`; the rules they follow are in `plan/ui.md` (Window layout).
- A part that isn't there is an empty `std::optional`, and `DeferredPositions::place` hides its window in the same
  batch. Nothing passes an infobar height yet, so that row stays empty until the infobar exists.
- The `move_*_splitter` functions turn the position a dragged splitter wants into new panel sizes, clamped to the
  limits.
- `MainWindow::layout_panels()` places every child through one `DeferredPositions` batch. It runs on `WM_SIZE`, after
  each splitter drag step (`on_*_splitter_drag` updates the `PanelState` first), and when View › Log flips the log.
- The window's minimum size is `MinimumWindowDips` (`WM_GETMINMAXINFO`).

## Saved state

`WindowState.cpp` stores one REG_DWORD per value under `HKCU\Software\WestieLib\Window`; the value names are the
`*Value` constants there.

- Bounds are `rcNormalPosition`: workspace coordinates in physical pixels, negative numbers as two's complement.
  Panel widths and the log height are DIPs; `LogVisible` and `Maximized` are 0 or 1.
- Without all four bounds the saved state is ignored. Other missing values fall back to their defaults.
- When the restored bounds land on a monitor with a different DPI, `WindowPlacement::restoring()` makes
  `on_dpi_changed` skip the suggested rect, because the saved bounds are already in that monitor's pixels, and only
  lay out again.
