# Main window

`MainWindow` (`src/ui/main/`) is the frame window. It owns the menu bar (`MainMenu.cpp`), the window icon, every
child window as a member (declaration order is creation order), its `WindowPlacement`, and the user's `PanelState`
(panel sizes in DIPs, and whether the log shows). It lays the children out and routes commands. What the window
should contain and how it should behave is specified in `plan/ui.md`; this file covers how the code is put together.

## Commands

- Every menu item, toolbar button and shortcut is a `CommandId` (`src/ui/commands/`). The values double as Win32
  command IDs.
- `CommandState` holds what the commands depend on (a sync running, the selection, whether the log shows).
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

1. The constructor creates the window hidden, creates its children, sets the icons and attaches the menu.
2. If `load_window_state()` finds saved state, the constructor takes the `PanelState` from it and calls
   `WindowPlacement::restore`, which runs `SetWindowPlacement` with `SW_HIDE` and the saved normal bounds. Without
   saved state, `apply_initial_size` uses `InitialWindowDips`, clamped to the monitor's work area.
3. `main.cpp` calls `show(nCmdShow)`. `WindowPlacement::show_command` turns `SW_SHOWNORMAL` / `SW_SHOWDEFAULT` into
   `SW_SHOWMAXIMIZED` when the saved window was maximized; any other show command, say from a shortcut, wins.
4. `WM_DESTROY` → `save_state()` → `PostQuitMessage(0)`. `WindowPlacement::current` reads `GetWindowPlacement` and
   also counts a window minimized with `WPF_RESTORETOMAXIMIZED` as maximized. A window torn down by an exception
   saves nothing, because `~Window` detaches before destroying.

## Toolbar and status bar

- `MainToolbar` (`src/ui/toolbar/`) is a strip window. It paints the white background and the bottom rule, and hosts
  a `Toolbar` control. A button click, or a choice from the Sync all dropdown, goes to the callback `MainWindow`
  gives it, which is `run_command`. `update(CommandState)` enables the buttons and keeps the state for the dropdown.
- `MainStatusBar` (`src/ui/statusbar/`) owns the `StatusBar`, its part widths, its text padding, and the part texts:
  Run, Tracks, Library and Tools from the left, an empty part, and Version against the right edge with its text
  centered. Until the core exists, Run reads `Ready` and Version shows the build's version. Library shows the folder
  `main.cpp` hands the window (`Music\WestieLib` for now), with a path ellipsis. The texts are owner-drawn, so
  `MainWindow` hands the status bar's `WM_DRAWITEM` to `MainStatusBar::draw`.
- Both measure their height in `apply_dpi`. `MainWindow::apply_dpi` makes the `FontSet` and hands the regular font
  and the DPI to both, and the set to the side panels. It runs in the constructor, on `WM_DPICHANGED`, and on
  `WM_SETTINGCHANGE` with `SPI_SETNONCLIENTMETRICS` (a new system font size).

## Side panels

- `PlaylistPanel` (`src/ui/playlists/`) stacks the `AllTracksRow`, a rule it paints itself, the `No playlists yet.`
  label and the `Add playlist` button. The label and the button start at `AllTracksRow::text_left()`, so they line up
  with the row's text at every scale. The button's click goes to the callback `MainWindow` gives the panel, which is
  `run_command`, as for the toolbar.
- `AllTracksRow` is a window that paints the row: selection box, glyph, name in semibold, and the counts. It stays
  selected until there are playlists to select instead.
- `InspectorPanel` (`src/ui/inspector/`) centers its first-run placeholder inside its padding on every `WM_SIZE`.
- `MainWindow::show_track_counts` hands `TrackCounts` to the All tracks row and the status bar. Until the core exists,
  the constructor shows zeros.

## Welcome screen

- `WelcomePanel` (`src/ui/welcome/`) is the main column while there are no playlists; `layout_panels` hides the
  `TrackPanel` meanwhile. It paints the app icon and lays out its labels, the `LibraryFolderField` and the default
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
- `MainWindow` gets the default folder from `main.cpp`. It hands it to the welcome panel, which starts the field at it
  and uses it in the hint's example, and to the status bar. The field's callback updates the status bar.

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
