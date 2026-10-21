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
| `Splitter` | Moves along `SplitterAxis::X` (a bar between columns) or `Y` (between rows). While dragging (with mouse capture), every mouse move calls its callback with the left or top edge it wants, in parent client pixels. The owner clamps that and lays out again. One window class serves both axes, so the cursor is set in `WM_SETCURSOR` |
| `DeferredPositions` | Collect `place(hwnd, rect)` calls, then `apply()`. A window gets `SWP_SHOWWINDOW` with a rect, and `SWP_HIDEWINDOW` with `std::nullopt`. It moves with `SWP_NOCOPYBITS`, then paints at once with `RedrawWindow(parent, RDW_UPDATENOW \| RDW_ALLCHILDREN)`. Those two fixed splitter drags that showed half-painted frames, so keep them. Every window in one batch must share a parent |
| `Dpi` | Sizes are DIPs in the code; `scale` / `unscale` convert at the point of use |
| `WindowIcon` | `WM_SETICON` doesn't hand the icons to the window, so this object keeps them alive. Call `apply_to` again after a DPI change |
| `Control` | Base for children of a system class (`Toolbar`, `StatusBar`). It subclasses the control, so it never destroys a handle its parent already took down (`WM_NCDESTROY`), and a derived class can override `handle_message` for the control's own messages. As in `Window`, exceptions there are parked. It can't be copied or moved. The control's notifications go to its parent, so the owning `Window` handles them |
| `Toolbar`, `StatusBar` | Created with `CCS_NORESIZE \| CCS_NOPARENTALIGN`, so the owner's layout places them like any other child |
| `Toolbar` | It measures its buttons only on `WM_SETFONT`, and adds its theme's margins to their content. So `apply_metrics` sets padding, gaps and separators first, sends the font last, and reaches the button height through vertical padding. Don't use `TB_SETBUTTONSIZE`: the default width it leaves behind clips the text of disabled buttons. The toolbar paints its own gray background and a dark, off-center separator; its parent fills the background at `CDDS_PREPAINT` and paints over `separator_rects()` at `CDDS_POSTPAINT` |
| `StatusBar` | `set_parts` takes fixed widths from the left and one part against the right edge, with a part between them taking up the slack. It fits them again on every `WM_SIZE`. The right part runs to the edge (`-1`) and has `SBT_NOBORDERS`, so no separator follows it. `SB_GETRECT` stops its rect at the size grip (or at the edge when the window is maximized and shows no grip), but the `WM_DRAWITEM` rect reaches under the grip, so `draw` clips to `SB_GETRECT`. The native text sits 3 px from a part's edge and can't be padded, so `set_text` makes the part owner-drawn and keeps the text with its alignment; the parent forwards `WM_DRAWITEM` to `draw`, which pads by `set_text_padding`. Owner-drawn parts have no text for screen readers |
| `SavedDc` | `SaveDC` / `RestoreDC`, for drawing into a DC someone else owns, such as an owner-draw item's |
| `MessageFont` | The message font for a DPI, with its line height. Controls keep the `HFONT` they were given, so on a change make the new font, hand it out, and only then replace the old one |
| `IconImageList` | An image list of icon resources at one pixel size. Controls don't own image lists: keep it alive while the control uses it, and rebuild it on a DPI change |
| `PaintScope` | `BeginPaint` / `EndPaint` for a `WM_PAINT` handler |
| `build_popup_menu` | Builds a popup from a `MenuItem` table; `MenuSeparator` (an empty item) is a separator. Item states aren't part of the table: the owner sets them when the popup opens |
| `AcceleratorTable` | Owns an `HACCEL`. `MessageLoop::run` translates through it for one window before `TranslateMessage`. `TranslateAccelerator` sends `WM_INITMENUPOPUP` first and skips grayed items, so shortcuts follow the menu's enabled state |
| `platform::win32::UniqueHandle<H, Close>` | `unique_ptr` with a function deleter. Add an alias next to the others for each new handle type |
| `platform::win32::to_utf16` | Core strings are UTF-8 `std::string` (see `plan/arch.md`); convert at the Win32 boundary |

## Adding a child window

1. Put the class in its area's folder and derive from `Panel`, or from `Window` with its own `window_class()`.
2. Make it a member of its owner, initialized with the owner's `hwnd()`. Declaration order is creation order.
3. Place it in the owner's layout pass through `DeferredPositions`.
4. Add the `.cpp` to `CMakeLists.txt`.

## DPI

The app is Per-Monitor V2. Sizes live in the code as DIPs and are scaled with `Dpi` where they're used. Top-level
windows handle `WM_DPICHANGED`: apply the suggested rect, reload DPI-dependent resources (icons, fonts, image lists),
and let `WM_SIZE` lay the children out again. Hairlines stay one physical pixel.

`main.cpp` calls `init_common_controls()` before creating any window.
