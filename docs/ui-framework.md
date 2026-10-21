# UI framework

Thin RAII classes over raw Win32; no MFC, ATL or WTL.

| Namespace | Folder | Holds |
|-----------|--------|-------|
| `westie::platform::win32` | `src/platform/win32/` | Win32 helpers that aren't UI-specific. Never includes `ui/` |
| `westie::ui::framework` | `src/ui/framework/` | reusable window building blocks |
| `westie::ui` | `src/ui/<area>/` | the app's windows, one folder per area of the window (`main/`, `playlists/`, …) |

## Errors

- `platform::win32::check(result, "ApiName")` returns `result`, or throws `std::system_error` with `GetLastError()`
  when it is zero or null. APIs that return an error code (the registry) use `check_status(status, "ApiName")`, and
  APIs that return an `HRESULT` use `check_hresult`.
- Exceptions never unwind through Win32 frames. `Window::window_proc` catches everything, parks it in
  `PendingException` and returns a failure value (`WM_NCCREATE` → `FALSE`, `WM_CREATE` → -1, otherwise 0).
  `MessageLoop::run` rethrows after each message, whether it was dispatched or translated as an accelerator, and the
  `Window` constructor rethrows after `CreateWindowExW`. Handlers simply throw; don't add local catches.
- `main.cpp` catches whatever reaches it and shows `show_fatal_error` (a message box), then exits with 1.

## Window lifecycle

- `WindowClass` registers its class on construction (window procedure `Window::window_proc`, extra bytes holding the
  `Window*`) and unregisters on destruction. Each window type keeps one as a function-local static in
  `static const framework::WindowClass& window_class()`, named `WestieLib.<Type>`.
- `Window` owns one HWND. Its protected constructor calls `CreateWindowExW` at `CW_USEDEFAULT`, so child windows start
  at 0×0 and their parent positions them. It can't be copied or moved, because the HWND stores `this`.
- **Construction rule.** Messages that arrive while the `Window` base is being constructed (`WM_NCCREATE`,
  `WM_CREATE`, the first `WM_SIZE`, …) go to `Window::handle_message`, which means `DefWindowProcW`: a virtual call
  during construction dispatches to the class being constructed. So don't rely on `WM_CREATE`; set up in the derived
  constructor's body instead. Child windows can be plain members initialized with `hwnd()`, because the base and its
  HWND exist before members are constructed. A handler that touches members must not be triggered during member
  initialization.
- **Destruction.** `~Window` detaches the object from the HWND, then calls `DestroyWindow`, so no handler runs on a
  half-destroyed object. A window that Windows destroys (closed by the user, or its parent went away) clears `hwnd_`
  on `WM_NCDESTROY`. Members are destroyed before the base, so child HWNDs go first.
- A window class overrides `handle_message`, switches on the message, calls small `on_*` methods, and passes the
  rest to `Window::handle_message`. `show` is virtual.
- Callers fill `CreateParams` with designated initializers, so each of its fields needs a default member initializer.

## Building blocks

The headers in `src/ui/framework/` and `src/platform/win32/` are short; read them for the full set. These carry
rules that the code doesn't spell out:

| Class | Rule |
|-------|------|
| `Panel` | Base for content panels: a child window with a `COLOR_WINDOW` background and `WS_CLIPCHILDREN \| WS_CLIPSIBLINGS`. Panels derive as `final`, with `explicit X(HWND parent) : Panel(parent) {}` |
| `Splitter` | While dragging (with mouse capture), every mouse move calls its callback with the position it wants, in parent client pixels. The owner clamps that and lays out again |
| `DeferredPositions` | Collect `move(hwnd, rect)` calls, then `apply()`. It moves with `SWP_NOCOPYBITS`, then paints at once with `RedrawWindow(parent, RDW_UPDATENOW \| RDW_ALLCHILDREN)`. Those two fixed splitter drags that showed half-painted frames, so keep them. Every window in one batch must share a parent |
| `Dpi` | Sizes are DIPs in the code; `scale` / `unscale` convert at the point of use |
| `WindowIcon` | `WM_SETICON` doesn't hand the icons to the window, so this object keeps them alive. Call `apply_to` again after a DPI change |
| `build_popup_menu` | Builds a popup from a `MenuItem` table; `MenuSeparator` (an empty item) is a separator. Item states aren't part of the table: the owner sets them when the popup opens |
| `AcceleratorTable` | Owns an `HACCEL`. `MessageLoop::run` translates through it for one window before `TranslateMessage`. `TranslateAccelerator` sends `WM_INITMENUPOPUP` first and skips grayed items, so shortcuts follow the menu's enabled state |
| `platform::win32::UniqueHandle<H, Close>` | `unique_ptr` with a function deleter. Add an alias next to the others for each new handle type |
| `platform::win32::to_utf16` | Core strings are UTF-8 `std::string` (see `plan/arch.md`); convert at the Win32 boundary |

## Adding a child window

1. Put the class in its area's folder and derive from `Panel`, or from `Window` with its own `window_class()`.
2. Make it a member of its owner, initialized with the owner's `hwnd()`. Declaration order is creation order.
3. Position it in the owner's layout pass through `DeferredPositions`.
4. Add the `.cpp` to `CMakeLists.txt`.

## DPI

The app is Per-Monitor V2. Sizes live in the code as DIPs and are scaled with `Dpi` where they're used. Top-level
windows handle `WM_DPICHANGED`: apply the suggested rect, reload DPI-dependent resources such as icons, and let
`WM_SIZE` lay the children out again.
