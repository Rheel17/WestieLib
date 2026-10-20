# Main window

`MainWindow` (`src/ui/main/`) is the frame window. It owns the menu bar (`MainMenu.cpp`), the window icon, every
child window as a member (declaration order is creation order), and the user's panel sizes in DIPs. What the window
should contain and how it should behave is specified in `plan/ui.md`; this file covers how the code is put together.

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

## Layout

- `MainLayout.cpp` is pure: it takes the client rect, the DPI and the panel sizes, and returns a rect per child, with
  no window calls. Size limits are constants in `MainLayout.h`; the rules they follow are in `plan/ui.md` (Window
  layout).
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
