#include "ui/main/MainWindow.h"

#include <chrono>
#include <optional>
#include <utility>
#include <variant>

#include "ResourceIds.h"
#include "core/playlists/AddPlaylist.h"
#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"
#include "ui/commands/CommandMenu.h"
#include "ui/dialogs/AboutDialog.h"
#include "ui/dialogs/AddPlaylistDialog.h"
#include "ui/dialogs/SettingsDialog.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Instance.h"
#include "ui/framework/Rect.h"
#include "ui/framework/Size.h"
#include "ui/framework/WindowClass.h"
#include "ui/main/MainMenu.h"
#include "ui/main/WindowState.h"

namespace westie::ui {

namespace {

constexpr framework::Size InitialWindowDips{1280, 800};
constexpr framework::Size MinimumWindowDips{1100, 680};

}

MainWindow::MainWindow(
        core::SettingsStore& settings_store, core::Settings settings, core::StateStore& state, CoreEventQueue& events, core::ToolManager& tools
) :
        Window(window_class(), {.title = L"WestieLib", .style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN}),
        settings_store_(settings_store),
        settings_(std::move(settings)),
        state_(state),
        events_(events),
        tools_(tools),
        placement_(hwnd()),
        icon_(IDI_APP_ICON),
        menu_bar_(hwnd(), GetSysColorBrush(COLOR_WINDOW)),
        fonts_(framework::Dpi::of(hwnd())),
        toolbar_(hwnd(), [this](CommandId command) { run_command(command); }),
        playlists_(hwnd(), [this](CommandId command) { run_command(command); }),
        playlists_splitter_(hwnd(), framework::SplitterAxis::X, [this](int left) { on_playlists_splitter_drag(left); }),
        tracks_(hwnd()),
        welcome_(
                hwnd(), settings_store.defaults().library_path, [this](CommandId command) { run_command(command); },
                [this](const std::filesystem::path& folder) { change_library_folder(folder); }
        ),
        inspector_splitter_(hwnd(), framework::SplitterAxis::X, [this](int left) { on_inspector_splitter_drag(left); }),
        inspector_(hwnd()),
        log_splitter_(hwnd(), framework::SplitterAxis::Y, [this](int top) { on_log_splitter_drag(top); }),
        log_(hwnd()),
        status_bar_(hwnd()) {
    apply_dpi(framework::Dpi::of(hwnd()));
    attach_menu();
    show_track_counts({});
    welcome_.set_library_folder(settings_.library_path);
    status_bar_.set_library_folder(settings_.library_path);

    if (std::optional<WindowState> state = load_window_state()) {
        panels_ = state->panels;
        placement_.restore(state->placement);
    } else {
        placement_.apply_initial_size(InitialWindowDips);
    }
    show_playlists();
    events_.set_target(hwnd());
}

void MainWindow::show(int show_command) {
    Window::show(placement_.show_command(show_command));
}

LRESULT MainWindow::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    if (menu_bar_.draw(message, lparam)) {
        return TRUE;
    }

    switch (message) {
    case CoreEventsMessage: on_core_events(); return 0;
    case WM_COMMAND:        on_command(wparam, lparam); return 0;
    case WM_INITMENUPOPUP:  on_init_menu_popup(reinterpret_cast<HMENU>(wparam), HIWORD(lparam) != 0); return Window::handle_message(message, wparam, lparam);
    case WM_DRAWITEM:       return on_draw_item(*reinterpret_cast<const DRAWITEMSTRUCT*>(lparam));
    case WM_GETMINMAXINFO:  on_get_min_max_info(*reinterpret_cast<MINMAXINFO*>(lparam)); return 0;
    case WM_SIZE:           layout_panels(); return 0;
    case WM_DPICHANGED:     on_dpi_changed(HIWORD(wparam), *reinterpret_cast<const RECT*>(lparam)); return 0;
    case WM_SETTINGCHANGE:  on_setting_change(wparam); return Window::handle_message(message, wparam, lparam);
    case WM_NCPAINT:
    case WM_NCACTIVATE:     return on_non_client_paint(message, wparam, lparam);
    case WM_THEMECHANGED:   menu_bar_.load_theme(); return Window::handle_message(message, wparam, lparam);
    case WM_DESTROY:        on_destroy(); return 0;
    default:                return Window::handle_message(message, wparam, lparam);
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

void MainWindow::apply_dpi(framework::Dpi dpi) {
    icon_.apply_to(hwnd(), dpi);
    menu_bar_.load_theme();

    // The controls keep using the old fonts until they have the new ones
    framework::FontSet fonts(dpi);
    toolbar_.apply_dpi(dpi, fonts.regular());
    playlists_.apply_dpi(dpi, fonts);
    welcome_.apply_dpi(dpi, fonts);
    inspector_.apply_fonts(fonts);
    status_bar_.apply_dpi(dpi, fonts.regular());
    fonts_ = std::move(fonts);
}

void MainWindow::show_track_counts(const TrackCounts& counts) {
    playlists_.set_track_counts(counts);
    status_bar_.set_track_count(counts.total);
}

void MainWindow::change_library_folder(const std::filesystem::path& folder) {
    settings_.library_path = folder;
    settings_store_.save(settings_);
    status_bar_.set_library_folder(folder);
}

bool MainWindow::has_playlists() const {
    return !state_.state().playlists.empty();
}

void MainWindow::add_playlist() {
    AddPlaylistDialog dialog([this](const core::PlaylistLink& link) { return state_.state().playlists.contains(link.key()); });
    std::optional<AddPlaylistRequest> request = dialog.choose(hwnd());
    // Nothing syncs yet, so Sync now has no effect
    if (request && core::add_playlist(state_, request->link, std::chrono::system_clock::now())) {
        show_playlists();
    }
}

void MainWindow::show_playlists() {
    playlists_.set_has_playlists(has_playlists());
    inspector_.show_placeholder(!has_playlists());
    layout_panels();
    // A hidden window would keep the keyboard focus from going anywhere
    if (IsChild(welcome_.hwnd(), GetFocus())) {
        SetFocus(hwnd());
    }
    update_commands();
}

void MainWindow::save_state() const {
    save_window_state({.placement = placement_.current(), .panels = panels_});
}

MainLayout MainWindow::current_layout() const {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    BarHeights bars{.toolbar = toolbar_.height(), .status_bar = status_bar_.height()};
    return compute_main_layout(framework::Rect::from_rect(client), framework::Dpi::of(hwnd()), bars, panels_);
}

void MainWindow::layout_panels() {
    MainLayout layout = current_layout();

    framework::DeferredPositions positions;
    positions.place(toolbar_.hwnd(), layout.toolbar);
    positions.place(playlists_.hwnd(), layout.playlists);
    positions.place(playlists_splitter_.hwnd(), layout.playlists_splitter);
    // Until there are playlists, the welcome screen takes the main column
    bool welcome = !has_playlists();
    positions.place(tracks_.hwnd(), welcome ? std::nullopt : std::optional(layout.tracks));
    positions.place(welcome_.hwnd(), welcome ? std::optional(layout.tracks) : std::nullopt);
    positions.place(inspector_splitter_.hwnd(), layout.inspector_splitter);
    positions.place(inspector_.hwnd(), layout.inspector);
    positions.place(log_splitter_.hwnd(), layout.log_splitter);
    positions.place(log_.hwnd(), layout.log);
    positions.place(status_bar_.hwnd(), layout.status_bar);
    positions.apply();
}

CommandState MainWindow::command_state() const {
    return {.tools_busy = tool_board_.running(), .has_playlists = has_playlists(), .log_visible = panels_.log_visible};
}

void MainWindow::update_commands() {
    toolbar_.update(command_state());
}

void MainWindow::run_command(CommandId command) {
    if (!command_state().enabled(command)) {
        return;
    }

    switch (command) {
    case CommandId::AddPlaylist:     add_playlist(); return;
    case CommandId::Exit:           SendMessageW(hwnd(), WM_CLOSE, 0, 0); return;
    case CommandId::ToggleLog:       toggle_log(); return;
    case CommandId::Settings:        show_settings(); return;
    case CommandId::CheckForUpdates: tools_.run(true); return;
    case CommandId::About:           show_about_dialog(hwnd()); return;
    default:                         return;
    }
}

void MainWindow::toggle_log() {
    panels_.log_visible = !panels_.log_visible;
    layout_panels();
    update_commands();
}

void MainWindow::show_settings() {
    // A folder typed into the welcome field takes effect when the field loses focus, which comes only after the dialog
    // has read the settings
    welcome_.commit_library_folder();
    settings_dialog_.emplace(settings_, tool_board_, [this] { run_command(CommandId::CheckForUpdates); });
    settings_dialog_->show_tools(command_state().enabled(CommandId::CheckForUpdates));
    std::optional<core::Settings> settings = settings_dialog_->edit(hwnd());
    settings_dialog_.reset();
    if (!settings || *settings == settings_) {
        return;
    }

    bool folder_changed = settings->library_path != settings_.library_path;
    settings_ = std::move(*settings);
    settings_store_.save(settings_);
    if (folder_changed) {
        status_bar_.set_library_folder(settings_.library_path);
        welcome_.set_library_folder(settings_.library_path);
    }
}

void MainWindow::show_tools() {
    status_bar_.set_tools(tool_board_);
    update_commands();
    if (settings_dialog_) {
        settings_dialog_->show_tools(command_state().enabled(CommandId::CheckForUpdates));
    }
}

void MainWindow::on_core_events() {
    for (const core::CoreEvent& event : events_.take()) {
        std::visit([this](const auto& payload) { apply_event(payload); }, event);
    }
    show_tools();
}

void MainWindow::apply_event(const core::ToolsRunChanged& event) {
    tool_board_.set_running(event.running);
}

void MainWindow::apply_event(const core::ToolStatus& status) {
    tool_board_.apply(status);
}

void MainWindow::apply_event(const core::ToolsChecked& event) {
    tool_board_.set_checked_at(event.checked_at);
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

LRESULT MainWindow::on_draw_item(const DRAWITEMSTRUCT& item) const {
    if (item.hwndItem != status_bar_.hwnd()) {
        return FALSE;
    }
    status_bar_.draw(item);
    return TRUE;
}

void MainWindow::on_playlists_splitter_drag(int left) {
    panels_ = move_playlists_splitter(current_layout(), framework::Dpi::of(hwnd()), panels_, left);
    layout_panels();
}

void MainWindow::on_inspector_splitter_drag(int left) {
    panels_ = move_inspector_splitter(current_layout(), framework::Dpi::of(hwnd()), panels_, left);
    layout_panels();
}

void MainWindow::on_log_splitter_drag(int top) {
    panels_ = move_log_splitter(current_layout(), framework::Dpi::of(hwnd()), panels_, top);
    layout_panels();
}

void MainWindow::on_get_min_max_info(MINMAXINFO& info) const {
    framework::Size minimum = framework::Dpi::of(hwnd()).scale(MinimumWindowDips);
    info.ptMinTrackSize = {minimum.width, minimum.height};
}

void MainWindow::on_dpi_changed(UINT dpi, const RECT& suggested) {
    apply_dpi(framework::Dpi(dpi));

    // Restored bounds are already in the pixels of the monitor they are on
    if (placement_.restoring()) {
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

void MainWindow::on_setting_change(WPARAM setting) {
    if (setting == SPI_SETNONCLIENTMETRICS) {
        apply_dpi(framework::Dpi::of(hwnd()));
        layout_panels();
    }
}

LRESULT MainWindow::on_non_client_paint(UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = Window::handle_message(message, wparam, lparam);
    menu_bar_.cover_edge();
    return result;
}

void MainWindow::on_destroy() {
    // The worker winds down while the window goes, and whatever it still reports stays in the queue
    tools_.stop();
    events_.set_target(nullptr);
    save_state();
    // A folder typed into the field takes effect when the field loses focus, which a close doesn't wait for
    welcome_.commit_library_folder();
    PostQuitMessage(0);
}

}
