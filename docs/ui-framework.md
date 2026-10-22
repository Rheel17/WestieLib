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
| `Panel` | Base for content panels: a child window with a `COLOR_WINDOW` background and `WS_CLIPCHILDREN \| WS_CLIPSIBLINGS`, plus any extra style it's given (`WS_VSCROLL` for the inspector). Panels derive as `final` and hand their parent and a name to `Panel(parent, name)`. The name is the window text: screen readers announce it, and tests find panels by it |
| `Splitter` | Moves along `SplitterAxis::X` (a bar between columns) or `Y` (between rows). While dragging (with mouse capture), every mouse move calls its callback with the left or top edge it wants, in parent client pixels. The owner clamps that and lays out again. One window class serves both axes, so the cursor is set in `WM_SETCURSOR` |
| `DeferredPositions` | Collect `place(hwnd, rect)` calls, then `apply()`. A window gets `SWP_SHOWWINDOW` with a rect, and `SWP_HIDEWINDOW` with `std::nullopt`. It moves with `SWP_NOCOPYBITS`, then paints at once with `RedrawWindow(parent, RDW_UPDATENOW \| RDW_ALLCHILDREN)`. Those two fixed splitter drags that showed half-painted frames, so keep them. Every window in one batch must share a parent |
| `VerticalScroll` | The vertical scroll position of a window that scrolls its own content (`WS_VSCROLL`). `update` takes the content's height, clamps the position to it and the client height, and sets the scroll bar; showing or hiding the bar sends `WM_SIZE`, so the owner's layout runs again inside it. `on_scroll` (`WM_VSCROLL`), `on_wheel` (`WM_MOUSEWHEEL`, in `SPI_GETWHEELSCROLLLINES` lines of 16 DIPs, small touchpad deltas added up to a notch), `scroll_to` and `reveal` (the least scroll that shows a span) return whether the position moved; the owner then lays out again, which calls `update` |
| `ItemTracker` | Hover and press for a window that paints its own items, named by index. The owner hit-tests and hands it the item under the mouse (or none) from `WM_MOUSEMOVE` (`move`, which also asks for `WM_MOUSELEAVE`), `WM_MOUSELEAVE` (`leave`), `WM_LBUTTONDOWN` (`press`, which takes the capture), `WM_LBUTTONUP` (`release`, which gives the capture back and returns the clicked item: the one the press started on, when the button comes up over it) and `WM_CAPTURECHANGED` (`cancel`). Each returns whether the looks changed, so the owner repaints. While the button is held, only the pressed item can be hot, and `pressed()` is set only while the mouse is over it |
| `FocusMemory` | Keeps the child that had the focus when a top-level window turned inactive, and gives it the focus again when the window gets `WM_SETFOCUS`, unless it's gone, hidden or disabled |
| `Dpi` | Sizes are DIPs in the code; `scale` / `unscale` convert at the point of use |
| `WindowIcon` | `WM_SETICON` doesn't hand the icons to the window, so this object keeps them alive. Call `apply_to` again after a DPI change |
| `Control` | Base for children of a system class (`Toolbar`, `StatusBar`). It subclasses the control, so it never destroys a handle its parent already took down (`WM_NCDESTROY`), and a derived class can override `handle_message` for the control's own messages. As in `Window`, exceptions there are parked. It can't be copied or moved. The control's notifications go to its parent, so the owning `Window` handles them. `CreateParams::id` is the child id that `WM_COMMAND` carries; buttons use their `CommandId` |
| `Toolbar`, `StatusBar` | Created with `CCS_NORESIZE \| CCS_NOPARENTALIGN`, so the owner's layout places them like any other child |
| `Toolbar` | It measures its buttons only on `WM_SETFONT`, and adds its theme's margins to their content. So `apply_metrics` sets padding, gaps and separators first, sends the font last, and reaches the button height through vertical padding. It measures a button's text only when the button gets its text, so after a new font (a DPI change) `apply_metrics` sets every text again; without that, labels end in "…" on a monitor with a larger scale. Don't use `TB_SETBUTTONSIZE`: the default width it leaves behind clips the text of disabled buttons. The toolbar paints its own gray background and a dark, off-center separator; its parent fills the background at `CDDS_PREPAINT` and paints over `separator_rects()` at `CDDS_POSTPAINT`. It draws a disabled button's text without ClearType, and neither `TBCDRF_NOETCHEDEFFECT` nor a custom draw text color changes that. So the parent asks for `CDDS_ITEMPOSTPAINT` on disabled buttons and calls `redraw_text`, which clears the text's area (after padding, icon and gap, up to a dropdown's arrow) and draws it again in the given color |
| `StatusBar` | `set_parts` takes `StatusParts`: parts from the left, parts against the right edge, and a part between them taking up the slack. Each `PartWidth` is a percentage of the bar's width with a minimum in pixels (a fixed part has only the minimum). When the window is too narrow for all of them, the left part named `yielding` narrows. It works the widths out again on every `WM_SIZE`. A part can be 0 wide; its separator then lies on the one before. The last part runs to the edge (`-1`) and has `SBT_NOBORDERS`, so no separator follows it. `SB_GETRECT` stops its rect at the size grip (or at the edge when the window is maximized and shows no grip), but the `WM_DRAWITEM` rect reaches under the grip, so `draw` pads from `SB_GETRECT`. `part_width_for` gives the width a part needs for a `PartContent`: its texts and icon with their gaps, the padding, and the gap the bar leaves between a part's edge and its rect (`SB_GETBORDERS`). The native text sits 3 px from a part's edge and can't be padded, so `set_text` and `set_content` make the part owner-drawn and keep its `PartContent` (a text, then optionally an icon and a trailing text, each a gap apart) with its `PartFormat` (alignment, and an ellipsis at the end or, for a path, in the middle); the parent forwards `WM_DRAWITEM` to `draw`, which pads and spaces by `set_spacing`. The last text is the one that gives way when the part is narrow. `set_tip` gives a part a tooltip, and `embed` places a child window (the progress bar) against a part's padded right edge, or hides it without a size or when the part is too narrow. Both follow the parts. Owner-drawn parts have no text for screen readers |
| `Tooltip` | A `TOOLTIPS_CLASS` popup with rect tools (`TTF_SUBCLASS`) on one window. `set_tool` adds a tool or updates its rect and text. Its maximum width is set, or it would ignore line breaks. Windows makes the top-level window its owner, whatever window it's created for |
| `ProgressBar` | A `PROGRESS_CLASS` child, 0–100, hidden until its owner shows it. Nothing uses it yet; it's for the Run part's sync progress, through `StatusBar::embed` |
| `MenuBarPainter` | Draws a window's menu bar, so the gray row the theme puts at its bottom goes away. Windows sends the undocumented `WM_UAHDRAWMENU` (background) and `WM_UAHDRAWMENUITEM` (each item, also on every hover and keyboard move) while it paints the bar; `draw` fills the background and draws the items with the `Menu` theme's `MENU_BARITEM` part and states. `cover_edge` paints over the line Windows draws below the bar, outside its rect; the owner calls it after the default `WM_NCPAINT` and `WM_NCACTIVATE`. Reload the theme on DPI and theme changes. Without visual styles (high contrast) it draws nothing and Windows draws the bar. `MIM_BACKGROUND` would be the documented route, but themed menu bars ignore it |
| `WindowDc` | `GetWindowDC` / `ReleaseDC`, for painting into the non-client area |
| `SavedDc` | `SaveDC` / `RestoreDC`, for drawing into a DC someone else owns, such as an owner-draw item's |
| `MessageFont` | The message font for a DPI, regular, semibold, as a title (semibold at 125%) or as a heading (semibold at 150%) (`FontStyle`), with its line height. Controls keep the `HFONT` they were given, so on a change make the new font, hand it out, and only then replace the old one |
| `FontSet` | Every `MessageFont` the UI uses, at one DPI. The owner of the windows that use it keeps it, and replaces it the same way |
| `Label` | Text that paints itself in a window of its own (`WestieLib.Label`): normal or gray, left or centered, one line with an ellipsis at the end or, for a path, in the middle (`TextEllipsis`, which the status bar shares), or wrapped. The text is also the window text, so tests and screen readers can read it. `height_for(width)` measures it. It paints through `BufferedPaint`. It isn't a `STATIC`, because a `STATIC` takes its colors from its parent's `WM_CTLCOLORSTATIC`, which would put color code in every panel |
| `Button` | A push button with a `CommandId`, optionally the default button, optionally with an icon; `set_enabled` grays it. `apply_dpi` sets the font and works out `size()`: at least 75×23 DIPs, else 10 DIPs of padding around the icon, a 5 DIP gap and the text. With an icon the button left-aligns its text (`BS_LEFT`) and draws the icon at the left of its theme's content margin, so the image list's left margin (the spare width's half, less that content margin) centers the row. Without that, the text centers in what the icon leaves, away from it |
| `LinkLabel` | A link that paints itself (`WestieLib.LinkLabel`): one line in the link color with an ellipsis, underlined while the mouse is over its text, with the hand cursor there. A click on the text (an `ItemTracker` press and release) calls its callback. It keeps an underlined copy of the font it's given. It isn't `WC_LINK`, which can't end in an ellipsis and takes its background from its parent |
| `Edit` | A single-line `EDIT` with a client edge, 23 DIPs tall or taller for the font. `apply_dpi` sets the margins after the font, because a new font resets them. Its notifications (`EN_CHANGE`, `EN_KILLFOCUS`, …) go to its parent |
| `IconImageList` | An image list of icon resources at one pixel size. Controls don't own image lists: keep it alive while the control uses it, and rebuild it on a DPI change. A list view destroys its image lists unless it has `LVS_SHAREIMAGELISTS`, so give it that |
| `IconResource` | One icon resource loaded at one pixel size, for windows that draw it themselves. Load it again after a DPI change |
| `PaintScope` | `BeginPaint` / `EndPaint` for a `WM_PAINT` handler |
| `Animation` | A `WM_TIMER` of the window it runs in, and a clock: `run(window, true)` starts both, `false` stops them. The window asks `is_tick` about every `WM_TIMER` and repaints on a tick, and the paint asks `progress(period)` how far into a period the animation is, from the time since it started, so late ticks don't slow it down. When Windows' animations are off (`SPI_GETCLIENTAREAANIMATION`) it runs without a timer, and the progress stays 0. Whatever repaints at every tick draws off screen first, or its text blinks |
| `paint_spinner` | Draws the busy spinner (`Spinner.cpp`) with GDI+, anti-aliased, in the progress ring's measures and colors, turned by a fraction of a full turn |
| `GdiPlusSession` | `GdiplusStartup` / `GdiplusShutdown`. `main.cpp` makes one before the window, and it outlives everything that draws with GDI+ |
| `BufferedPaint` | A `PaintScope` that hands out a buffer for the area to paint, at the same client coordinates, and copies it to the window in one `BitBlt` at the end. Windows that paint their own content use it, and fill their own background instead of erasing it: no class background brush, or `TRUE` from `WM_ERASEBKGND`. Each splitter drag step repaints every panel it touches, so an erase followed by a paint shows as blinking text |
| `ModalDialog` | Base for dialogs made from a `DIALOGEX` template in the `.rc`. Per-Monitor V2 scales a template dialog and its font on its own, so dialogs need no DIP code. `show_modal` runs `DialogBoxParamW`; the object is fully built by then, so unlike a `Window` it gets `WM_INITDIALOG` through its virtual `handle_message` (return `TRUE` / `FALSE` as a dialog procedure does). The dialog centers on its owner, as the TaskDialogs do. The dialog's own message loop doesn't look for parked exceptions, so a handler that throws ends the dialog with `IDCANCEL`, and `show_modal` rethrows |
| `build_popup_menu` | Builds a popup from a `MenuItem` table; `MenuSeparator` (an empty item) is a separator. Item states aren't part of the table: the owner sets them when the popup opens |
| `AcceleratorTable` | Owns an `HACCEL`. `MessageLoop::run` translates through it for one window before `TranslateMessage`. `TranslateAccelerator` sends `WM_INITMENUPOPUP` first and skips grayed items, so shortcuts follow the menu's enabled state |
| `platform::win32::UniqueHandle<H, Close>` | `unique_ptr` with a function deleter. Add an alias next to the others for each new handle type. `UniqueComPtr<I>` releases a COM interface |
| `platform::win32::ComApartment` | Joins the UI thread to a single-threaded COM apartment for as long as it lives. `main.cpp` makes one before any window, because the shell's dialogs (`pick_folder`) need it |
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

`main.cpp` makes the `ComApartment` and calls `init_common_controls()` before creating any window.
