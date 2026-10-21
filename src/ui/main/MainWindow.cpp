#include "ui/main/MainWindow.h"

#include <algorithm>
#include <optional>

#include "ResourceIds.h"
#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"
#include "ui/commands/CommandMenu.h"
#include "ui/dialogs/AboutDialog.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Instance.h"
#include "ui/framework/Rect.h"
#include "ui/framework/Size.h"
#include "ui/framework/WindowClass.h"
#include "ui/main/MainMenu.h"

namespace westie::ui {

namespace {

constexpr framework::Size InitialWindowDips{1280, 800};
constexpr framework::Size MinimumWindowDips{1100, 680};

}

MainWindow::MainWindow() :
        Window(window_class(), {.title = L"WestieLib", .style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN}),
        icon_(IDI_APP_ICON),
        playlists_(hwnd()),
        playlists_splitter_(hwnd(), [this](int left) { on_playlists_splitter_drag(left); }),
        tracks_(hwnd()),
        inspector_splitter_(hwnd(), [this](int left) { on_inspector_splitter_drag(left); }),
        inspector_(hwnd()) {
    icon_.apply_to(hwnd(), framework::Dpi::of(hwnd()));
    attach_menu();

    if (std::optional<WindowState> state = load_window_state()) {
        widths_ = state->panel_widths;
        start_maximized_ = state->maximized;
        restore_placement(*state);
    } else {
        apply_initial_size();
    }
}

void MainWindow::show(int show_command) {
    bool default_show = show_command == SW_SHOWNORMAL || show_command == SW_SHOWDEFAULT;
    Window::show(start_maximized_ && default_show ? SW_SHOWMAXIMIZED : show_command);
}

LRESULT MainWindow::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_COMMAND:       on_command(wparam, lparam); return 0;
    case WM_INITMENUPOPUP: on_init_menu_popup(reinterpret_cast<HMENU>(wparam), HIWORD(lparam) != 0); return Window::handle_message(message, wparam, lparam);
    case WM_GETMINMAXINFO: on_get_min_max_info(*reinterpret_cast<MINMAXINFO*>(lparam)); return 0;
    case WM_SIZE:          layout_panels(); return 0;
    case WM_DPICHANGED:    on_dpi_changed(HIWORD(wparam), *reinterpret_cast<const RECT*>(lparam)); return 0;
    case WM_DESTROY:       on_destroy(); return 0;
    default:               return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& MainWindow::window_class() {
    static framework::WindowClass window_class({
        .name = L"WestieLib.MainWindow",
        .background = GetSysColorBrush(COLOR_BTNFACE),
        .icon = LoadIconW(framework::module_instance(), MAKEINTRESOURCEW(IDI_APP_ICON)),
    });
    return window_class;
}

void MainWindow::attach_menu() {
    platform::win32::UniqueMenu menu = build_main_menu();
    platform::win32::check(SetMenu(hwnd(), menu.get()), "SetMenu");
    // The window owns it now
    menu.release();
}

void MainWindow::apply_initial_size() {
    framework::Size size = framework::Dpi::of(hwnd()).scale(InitialWindowDips);

    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    platform::win32::check(GetMonitorInfoW(MonitorFromWindow(hwnd(), MONITOR_DEFAULTTONEAREST), &monitor), "GetMonitorInfoW");
    const RECT& work = monitor.rcWork;
    size.width = std::min(size.width, static_cast<int>(work.right - work.left));
    size.height = std::min(size.height, static_cast<int>(work.bottom - work.top));

    RECT window;
    platform::win32::check(GetWindowRect(hwnd(), &window), "GetWindowRect");
    int x = std::clamp(static_cast<int>(window.left), static_cast<int>(work.left), static_cast<int>(work.right) - size.width);
    int y = std::clamp(static_cast<int>(window.top), static_cast<int>(work.top), static_cast<int>(work.bottom) - size.height);
    platform::win32::check(SetWindowPos(hwnd(), nullptr, x, y, size.width, size.height, SWP_NOZORDER | SWP_NOACTIVATE), "SetWindowPos");
}

void MainWindow::restore_placement(const WindowState& state) {
    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);
    placement.showCmd = SW_HIDE;
    placement.rcNormalPosition = state.normal_bounds.to_rect();

    restoring_placement_ = true;
    BOOL restored = SetWindowPlacement(hwnd(), &placement);
    restoring_placement_ = false;
    platform::win32::check(restored, "SetWindowPlacement");
}

void MainWindow::save_state() const {
    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);
    platform::win32::check(GetWindowPlacement(hwnd(), &placement), "GetWindowPlacement");

    bool minimized_from_maximized = placement.showCmd == SW_SHOWMINIMIZED && (placement.flags & WPF_RESTORETOMAXIMIZED);
    save_window_state({
        .normal_bounds = framework::Rect::from_rect(placement.rcNormalPosition),
        .maximized = placement.showCmd == SW_SHOWMAXIMIZED || minimized_from_maximized,
        .panel_widths = widths_,
    });
}

MainLayout MainWindow::current_layout() const {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    return compute_main_layout(framework::Rect::from_rect(client), framework::Dpi::of(hwnd()), widths_);
}

void MainWindow::layout_panels() {
    MainLayout layout = current_layout();

    framework::DeferredPositions positions;
    positions.move(playlists_.hwnd(), layout.playlists);
    positions.move(playlists_splitter_.hwnd(), layout.playlists_splitter);
    positions.move(tracks_.hwnd(), layout.tracks);
    positions.move(inspector_splitter_.hwnd(), layout.inspector_splitter);
    positions.move(inspector_.hwnd(), layout.inspector);
    positions.apply();
}

CommandState MainWindow::command_state() const {
    return {};
}

void MainWindow::run_command(CommandId command) {
    if (!command_state().enabled(command)) {
        return;
    }

    switch (command) {
    case CommandId::Exit:  SendMessageW(hwnd(), WM_CLOSE, 0, 0); return;
    case CommandId::About: show_about_dialog(hwnd()); return;
    default:               return;
    }
}

void MainWindow::on_command(WPARAM wparam, LPARAM lparam) {
    // Only menus and accelerators send WM_COMMAND without a control
    if (lparam == 0) {
        run_command(static_cast<CommandId>(LOWORD(wparam)));
    }
}

void MainWindow::on_init_menu_popup(HMENU menu, bool window_menu) const {
    if (!window_menu) {
        update_menu_items(menu, command_state());
    }
}

void MainWindow::on_playlists_splitter_drag(int left) {
    widths_ = move_playlists_splitter(current_layout(), framework::Dpi::of(hwnd()), widths_, left);
    layout_panels();
}

void MainWindow::on_inspector_splitter_drag(int left) {
    widths_ = move_inspector_splitter(current_layout(), framework::Dpi::of(hwnd()), widths_, left);
    layout_panels();
}

void MainWindow::on_get_min_max_info(MINMAXINFO& info) const {
    framework::Size minimum = framework::Dpi::of(hwnd()).scale(MinimumWindowDips);
    info.ptMinTrackSize = {minimum.width, minimum.height};
}

void MainWindow::on_dpi_changed(UINT dpi, const RECT& suggested) {
    icon_.apply_to(hwnd(), framework::Dpi(dpi));

    // Restored bounds are already in the pixels of the monitor they are on
    if (restoring_placement_) {
        layout_panels();
        return;
    }
    platform::win32::check(
            SetWindowPos(
                    hwnd(), nullptr, suggested.left, suggested.top, suggested.right - suggested.left, suggested.bottom - suggested.top,
                    SWP_NOZORDER | SWP_NOACTIVATE
            ),
            "SetWindowPos"
    );
}

void MainWindow::on_destroy() {
    save_state();
    PostQuitMessage(0);
}

}
