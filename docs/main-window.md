# Main window

`MainWindow` (`src/ui/main/`) is the frame window. It owns the menu bar (`MainMenu.cpp`), the window icon, every
child window as a member (declaration order is creation order), and the user's panel sizes in DIPs. It also routes
commands. What the window should contain and how it should behave is specified in `plan/ui.md`; this file covers how
the code is put together.

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
- Menus and accelerators arrive as `WM_COMMAND` with no control. `run_command` drops a command that
  `command_state()` says is disabled, then acts on it. Commands without a feature yet do nothing.

## Startup and shutdown

1. The constructor creates the window hidden, creates its children, sets the icons and attaches the menu.
2. If `load_window_state()` finds saved state, the constructor takes the panel sizes from it and calls
   `restore_placement`, which runs `SetWindowPlacement` with `SW_HIDE` and the saved normal bounds. Without saved
   state, `apply_initial_size()` uses `InitialWindowDips`, clamped to the monitor's work area.
3. `main.cpp` calls `show(nCmdShow)`. `MainWindow::show` turns `SW_SHOWNORMAL` / `SW_SHOWDEFAULT` into
   `SW_SHOWMAXIMIZED` when the saved state was maximized; any other show command, say from a shortcut, wins.
4. `WM_DESTROY` → `save_state()` → `PostQuitMessage(0)`. `save_state` reads `GetWindowPlacement` and also counts a
   window minimized with `WPF_RESTORETOMAXIMIZED` as maximized. A window torn down by an exception saves nothing,
   because `~Window` detaches before destroying.

## Toolbar and status bar

- `MainToolbar` (`src/ui/toolbar/`) is a strip window. It paints the white background and the bottom rule, and hosts
  a `Toolbar` control. A button click, or a choice from the Sync all dropdown, goes to the callback `MainWindow`
  gives it, which is `run_command`. `update(CommandState)` enables the buttons and keeps the state for the dropdown.
- `MainStatusBar` (`src/ui/statusbar/`) owns the `StatusBar`, its part widths, its text padding, and the part texts:
  Run, Library and Tools from the left, an empty part, and Version against the right edge with its text centered.
  Until the core exists, Run reads `Ready` and Version shows the build's version. The texts are owner-drawn, so
  `MainWindow` hands the status bar's `WM_DRAWITEM` to `MainStatusBar::draw`.
- Both measure their height in `apply_dpi`. `MainWindow::apply_dpi` makes the message font and hands it and the DPI
  to both. It runs in the constructor, on `WM_DPICHANGED`, and on `WM_SETTINGCHANGE` with `SPI_SETNONCLIENTMETRICS`
  (a new system font size).

## Layout

- `MainLayout.cpp` is pure: it takes the client rect, the DPI, the bar heights in pixels and the panel sizes, and
  returns a rect per child, with no window calls. The toolbar takes the top, the status bar the bottom, and the
  panels share the rest. Size limits are constants in `MainLayout.h`; the rules they follow are in `plan/ui.md`
  (Window layout).
- The `move_*_splitter` functions turn the position a dragged splitter wants into new panel sizes, clamped to the
  limits.
- `MainWindow::layout_panels()` moves every child through one `DeferredPositions` batch. It runs on `WM_SIZE` and
  after each splitter drag step (`on_*_splitter_drag` updates the sizes first).
- The window's minimum size is `MinimumWindowDips` (`WM_GETMINMAXINFO`).

## Saved state

`WindowState.cpp` stores one REG_DWORD per value under `HKCU\Software\WestieLib\Window`; the value names are the
`*Value` constants there.

- Bounds are `rcNormalPosition`: workspace coordinates in physical pixels, negative numbers as two's complement.
  Panel sizes are DIPs.
- Without all four bounds the saved state is ignored. Other missing values fall back to their defaults.
- When the restored bounds land on a monitor with a different DPI, `restoring_placement_` makes `on_dpi_changed`
  skip the suggested rect, because the saved bounds are already in that monitor's pixels, and only lay out again.
