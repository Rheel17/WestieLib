#include "ui/main/MainWindow.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "ResourceIds.h"
#include "core/matching/ConfirmMatch.h"
#include "core/matching/YtmOverride.h"
#include "core/sources/SourceType.h"
#include "core/playlists/AddPlaylist.h"
#include "core/playlists/RemovePlaylist.h"
#include "platform/NearestExistingFolder.h"
#include "platform/win32/Clipboard.h"
#include "platform/win32/ShellOpen.h"
#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"
#include "ui/commands/CommandMenu.h"
#include "ui/dialogs/AboutDialog.h"
#include "ui/dialogs/AddPlaylistDialog.h"
#include "ui/dialogs/ChooseMatchDialog.h"
#include "ui/dialogs/MatchChoice.h"
#include "ui/dialogs/RemovePlaylistDialog.h"
#include "ui/dialogs/SettingsDialog.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Instance.h"
#include "ui/framework/Rect.h"
#include "ui/framework/Size.h"
#include "ui/framework/WindowClass.h"
#include "ui/inspector/TrackDetails.h"
#include "ui/log/SelectedForLog.h"
#include "ui/main/MainMenu.h"
#include "ui/main/WindowState.h"
#include "ui/playlists/PlaylistMenu.h"
#include "ui/playlists/PlaylistRows.h"
#include "ui/tracks/TrackRows.h"

namespace westie::ui {

namespace {

constexpr framework::Size InitialWindowDips{1280, 800};
constexpr framework::Size MinimumWindowDips{1100, 680};

}

MainWindow::MainWindow(
        core::SettingsStore& settings_store,
        core::Settings settings,
        core::StateStore& state,
        CoreEventQueue& events,
        core::ToolManager& tools,
        core::PlaylistLookup& lookup,
        core::SyncEngine& sync,
        core::CandidateSearch& candidate_search,
        const platform::TextNormalizer& normalizer,
        core::Log& logger
) :
        Window(window_class(), {.title = L"WestieLib", .style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN}),
        settings_store_(settings_store),
        settings_(std::move(settings)),
        state_(state),
        events_(events),
        tools_(tools),
        lookup_(lookup),
        sync_(sync),
        candidate_search_(candidate_search),
        normalizer_(normalizer),
        logger_(logger),
        placement_(hwnd()),
        icon_(IDI_APP_ICON),
        menu_bar_(hwnd(), GetSysColorBrush(COLOR_WINDOW)),
        fonts_(framework::Dpi::of(hwnd())),
        toolbar_(hwnd(), [this](CommandId command) { run_command(command); }),
        infobar_(
                hwnd(), [this] { show_log(); }, [this] { dismiss_fetch_failure(); }
        ),
        playlists_(
                hwnd(),
                {
                    .on_command = [this](CommandId command) { run_command(command); },
                    .on_select = [this](std::optional<std::string> key) { select_playlist(std::move(key)); },
                    .on_context_menu = [this](POINT screen) { show_playlist_menu(screen); },
                    .on_sync = [this](const std::string& key) { sync_playlist(key); },
                }
        ),
        playlists_splitter_(hwnd(), framework::SplitterAxis::X, [this](int left) { on_playlists_splitter_drag(left); }),
        tracks_(hwnd(), normalizer, [this] { show_inspector(); }),
        welcome_(
                hwnd(),
                settings_store.defaults().library_path,
                [this](CommandId command) { run_command(command); },
                [this](const std::filesystem::path& folder) { change_library_folder(folder); }
        ),
        inspector_splitter_(hwnd(), framework::SplitterAxis::X, [this](int left) { on_inspector_splitter_drag(left); }),
        inspector_(
                hwnd(),
                {
                    .on_command = [this](CommandId command) { run_command(command); },
                    .on_open_url = [this](const std::wstring& url) { platform::win32::open_with_shell(hwnd(), url); },
                    .on_select_playlist = [this](const std::string& key) { select_playlist(key); },
                }
        ),
        log_splitter_(hwnd(), framework::SplitterAxis::Y, [this](int top) { on_log_splitter_drag(top); }),
        log_(
                hwnd(),
                {
                    .on_open_folder = [this] { run_command(CommandId::OpenLogFolder); },
                    .on_close = [this] { run_command(CommandId::ToggleLog); },
                }
        ),
        status_bar_(hwnd()) {
    apply_dpi(framework::Dpi::of(hwnd()));
    attach_menu();
    welcome_.set_library_folder(settings_.library_path);
    status_bar_.set_library_folder(settings_.library_path);

    if (std::optional<WindowState> state = load_window_state()) {
        panels_ = state->panels;
        tracks_.set_column_widths(state->column_dips);
        log_.set_view(state->log_view);
        choose_match_dips_ = state->choose_match_dips.value_or(ChooseMatchDialog::DefaultWindowDips);
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
    case WM_ACTIVATE:       on_activate(LOWORD(wparam)); return Window::handle_message(message, wparam, lparam);
    case WM_SETFOCUS:       focus_.restore(hwnd()); return 0;
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
    infobar_.apply_dpi(dpi, fonts.regular());
    playlists_.apply_dpi(dpi, fonts);
    tracks_.apply_dpi(dpi, fonts);
    welcome_.apply_dpi(dpi, fonts);
    inspector_.apply_dpi(dpi, fonts);
    log_.apply_dpi(dpi, fonts);
    status_bar_.apply_dpi(dpi, fonts.regular());
    fonts_ = std::move(fonts);
}

void MainWindow::show_track_counts(const core::State& state) {
    track_counts_ = count_tracks(state, std::nullopt);
    playlists_.set_track_counts(track_counts_);
    status_bar_.set_track_count(track_counts_.total);
}

void MainWindow::change_library_folder(const std::filesystem::path& folder) {
    settings_.library_path = folder;
    settings_store_.save(settings_);
    status_bar_.set_library_folder(folder);
    show_inspector();
}

bool MainWindow::has_playlists() const {
    return !state_.snapshot()->playlists.empty();
}

std::optional<std::string> MainWindow::added_playlist_name(const core::PlaylistLink& link) const {
    std::shared_ptr<const core::State> state = state_.snapshot();
    auto playlist = state->playlists.find(link.key());
    if (playlist == state->playlists.end()) {
        return std::nullopt;
    }
    return playlist->second.name;
}

void MainWindow::add_playlist() {
    add_playlist_dialog_.emplace([this](const core::PlaylistLink& link) { return added_playlist_name(link); }, lookup_, tool_board_);
    std::optional<AddPlaylistRequest> request = add_playlist_dialog_->choose(hwnd());
    add_playlist_dialog_.reset();
    if (!request || !core::add_playlist(state_, logger_, request->link, std::move(request->name), std::chrono::system_clock::now())) {
        return;
    }
    selected_playlist_ = request->link.key();
    show_playlists();
    if (request->sync_now) {
        sync_.sync({request->link.key()});
    }
}

void MainWindow::select_playlist(std::optional<std::string> key) {
    selected_playlist_ = std::move(key);
    playlists_.show_selection(selected_playlist_);
    show_scope();
    update_commands();
}

void MainWindow::show_scope() {
    std::shared_ptr<const core::State> state = state_.snapshot();
    std::optional<PlaylistRow> playlist = selected_playlist_ ? playlist_row(*state, *selected_playlist_) : std::nullopt;
    show_track_counts(*state);
    tracks_.show_tracks(playlist, track_rows(*state, selected_playlist_, normalizer_), track_counts_.total);
    show_inspector();
}

void MainWindow::show_inspector() {
    std::vector<TrackRow> selected = tracks_.selected_rows();
    log_.set_selection(selected_for_log(*state_.snapshot(), selected_playlist_, selected));
    if (!has_playlists()) {
        inspector_.show_first_run();
        return;
    }
    if (selected.size() == 1) {
        if (std::optional<TrackDetails> track = track_details(*state_.snapshot(), normalizer_, selected.front(), settings_.library_path)) {
            inspector_.show_track(*track);
            return;
        }
    }
    if (selected.size() > 1) {
        inspector_.show_selection(status_counts(selected));
        return;
    }
    if (std::optional<PlaylistRow> playlist = selected_row()) {
        inspector_.show_playlist(*playlist);
    } else {
        inspector_.show_library(settings_.library_path, track_counts_);
    }
}

void MainWindow::show_playlists() {
    std::shared_ptr<const core::State> state = state_.snapshot();
    if (selected_playlist_ && !state->playlists.contains(*selected_playlist_)) {
        selected_playlist_.reset();
    }
    std::vector<PlaylistRow> rows = playlist_rows(*state);
    mark_runs(rows, sync_board_);
    show_run(rows);
    playlists_.set_playlists(std::move(rows));
    playlists_.show_selection(selected_playlist_);
    show_scope();
    layout_panels();
    // A hidden window would keep the keyboard focus from going anywhere
    if (IsChild(welcome_.hwnd(), GetFocus())) {
        SetFocus(hwnd());
    }
    update_commands();
}

void MainWindow::show_run(const std::vector<PlaylistRow>& rows) {
    std::optional<std::chrono::system_clock::time_point> last_sync;
    for (const PlaylistRow& row : rows) {
        if (row.last_sync && (!last_sync || *row.last_sync > *last_sync)) {
            last_sync = row.last_sync;
        }
    }
    status_bar_.set_run(sync_board_, last_sync);

    fetch_failure_.update(rows);
    if (const std::optional<std::wstring>& message = fetch_failure_.message()) {
        infobar_.set_message(*message);
    }
}

void MainWindow::save_state() const {
    save_window_state({
            .placement = placement_.current(),
            .panels = panels_,
            .column_dips = tracks_.column_widths(),
            .log_view = log_.view(),
            .choose_match_dips = choose_match_dips_,
    });
}

MainLayout MainWindow::current_layout() const {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    BarHeights bars{.toolbar = toolbar_.height(), .infobar = fetch_failure_.message() ? infobar_.height() : 0, .status_bar = status_bar_.height()};
    return compute_main_layout(framework::Rect::from_rect(client), framework::Dpi::of(hwnd()), bars, panels_);
}

void MainWindow::layout_panels() {
    MainLayout layout = current_layout();

    framework::DeferredPositions positions;
    positions.place(toolbar_.hwnd(), layout.toolbar);
    positions.place(infobar_.hwnd(), layout.infobar);
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
    std::vector<TrackRow> selected = tracks_.selected_rows();
    return {
        .syncing = sync_board_.running(),
        .tools_busy = tool_board_.running(),
        .has_playlists = has_playlists(),
        .playlist_selected = selected_playlist_.has_value(),
        .review_selected = selected.size() == 1 && selected.front().status == StatusTab::NeedsReview,
        .spotify_entry_selected = selected.size() == 1 && selected.front().source == core::SourceType::Spotify,
        .failed_selected = std::ranges::any_of(selected, [](const TrackRow& row) { return row.counts_as == StatusTab::Failed; }),
        .file_selected = selected.size() == 1 && !selected.front().file.empty(),
        .log_visible = panels_.log_visible,
    };
}

void MainWindow::update_commands() {
    toolbar_.update(command_state());
    inspector_.update(command_state());
    // The rows' update controls follow Update all: any playlist, while no update runs
    playlists_.set_sync_enabled(command_state().enabled(CommandId::UpdateAll));
}

void MainWindow::run_command(CommandId command) {
    if (!command_state().enabled(command)) {
        return;
    }

    switch (command) {
    case CommandId::AddPlaylist:            add_playlist(); return;
    case CommandId::RemovePlaylist:         remove_playlist(); return;
    case CommandId::OpenLibraryFolder:      open_library_folder(); return;
    case CommandId::OpenLogFolder:          open_log_folder(); return;
    case CommandId::OpenPlaylistPage:       open_playlist_page(); return;
    case CommandId::CopyPlaylistLink:       copy_playlist_link(); return;
    case CommandId::UpdateAll:              sync_all(); return;
    case CommandId::UpdateSelectedPlaylist: sync_.sync({*selected_playlist_}); return;
    case CommandId::CancelUpdate:           sync_.cancel(); return;
    case CommandId::Exit:                   SendMessageW(hwnd(), WM_CLOSE, 0, 0); return;
    case CommandId::ToggleLog:              toggle_log(); return;
    case CommandId::Settings:               show_settings(); return;
    case CommandId::CheckForUpdates:        tools_.run(true); return;
    case CommandId::About:                  show_about_dialog(hwnd()); return;
    case CommandId::FocusSearch:            tracks_.focus_search(); return;
    case CommandId::ConfirmMatch:           confirm_match(); return;
    case CommandId::ChooseMatch:            choose_match(); return;
    case CommandId::Retry:                  retry_selected(); return;
    case CommandId::ShowInFolder:           show_selected_in_folder(); return;
    default:                                return;
    }
}

void MainWindow::sync_playlist(const std::string& key) {
    if (command_state().enabled(CommandId::UpdateAll)) {
        sync_.sync({key});
    }
}

void MainWindow::sync_all() {
    std::vector<std::string> keys;
    for (const PlaylistRow& row : playlist_rows(*state_.snapshot())) {
        keys.push_back(row.key);
    }
    sync_.sync(keys);
}

std::optional<PlaylistRow> MainWindow::selected_row() const {
    return selected_playlist_ ? playlist_row(*state_.snapshot(), *selected_playlist_) : std::nullopt;
}

void MainWindow::show_playlist_menu(POINT screen) {
    if (std::optional<CommandId> command = choose_playlist_command(hwnd(), screen, command_state())) {
        run_command(*command);
    }
}

void MainWindow::remove_playlist() {
    std::optional<PlaylistRow> playlist = selected_row();
    if (!playlist || !confirm_remove_playlist(hwnd(), playlist->name)) {
        return;
    }
    core::remove_playlist(state_, logger_, playlist->key);
    selected_playlist_.reset();
    show_playlists();
}

void MainWindow::confirm_match() {
    std::vector<TrackRow> selected = tracks_.selected_rows();
    if (selected.size() != 1) {
        return;
    }
    core::confirm_match(state_, logger_, selected.front().track);
    show_playlists();
}

void MainWindow::choose_match() {
    std::vector<TrackRow> selected = tracks_.selected_rows();
    if (selected.size() != 1) {
        return;
    }
    std::optional<MatchChoice> choice = match_choice(*state_.snapshot(), selected.front().key);
    if (!choice) {
        return;
    }
    std::string source = choice->source;
    choose_match_dialog_.emplace(
            std::move(*choice),
            candidate_search_,
            [this](const std::string& ytm_id) { return library_result_title(*state_.snapshot(), ytm_id); },
            choose_match_dips_
    );
    std::optional<std::string> ytm_id = choose_match_dialog_->choose(hwnd());
    choose_match_dips_ = choose_match_dialog_->window_dips();
    choose_match_dialog_.reset();
    if (!ytm_id) {
        return;
    }
    core::set_ytm_override(state_, logger_, source, *ytm_id);
    show_playlists();
}

void MainWindow::retry_selected() {
    std::vector<std::string> tracks;
    for (const TrackRow& row : tracks_.selected_rows()) {
        if (row.counts_as == StatusTab::Failed && !std::ranges::contains(tracks, row.track)) {
            tracks.push_back(row.track);
        }
    }
    sync_.retry(tracks);
    show_playlists();
}

void MainWindow::show_selected_in_folder() const {
    std::vector<TrackRow> selected = tracks_.selected_rows();
    if (selected.size() == 1 && !selected.front().file.empty() && !platform::win32::show_in_folder(settings_.library_path / selected.front().file)) {
        MessageBeep(MB_ICONWARNING);
    }
}

void MainWindow::open_library_folder() const {
    // The first sync creates the folder, so until then its nearest existing parent opens
    if (std::optional<std::filesystem::path> folder = platform::nearest_existing_folder(settings_.library_path)) {
        platform::win32::open_with_shell(hwnd(), folder->wstring());
    }
}

void MainWindow::open_log_folder() const {
    if (std::optional<std::filesystem::path> folder = platform::nearest_existing_folder(logger_.folder())) {
        platform::win32::open_with_shell(hwnd(), folder->wstring());
    }
}

void MainWindow::open_playlist_page() const {
    if (std::optional<PlaylistRow> playlist = selected_row()) {
        platform::win32::open_with_shell(hwnd(), playlist->url);
    }
}

void MainWindow::copy_playlist_link() const {
    std::optional<PlaylistRow> playlist = selected_row();
    if (playlist && !platform::win32::set_clipboard_text(hwnd(), playlist->url)) {
        MessageBeep(MB_ICONWARNING);
    }
}

void MainWindow::toggle_log() {
    panels_.log_visible = !panels_.log_visible;
    layout_panels();
    update_commands();
}

void MainWindow::show_log() {
    if (!panels_.log_visible) {
        toggle_log();
    }
}

void MainWindow::dismiss_fetch_failure() {
    fetch_failure_.dismiss();
    show_playlists();
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
        show_inspector();
    }
}

void MainWindow::show_tools() {
    status_bar_.set_tools(tool_board_);
    update_commands();
    if (settings_dialog_) {
        settings_dialog_->show_tools(command_state().enabled(CommandId::CheckForUpdates));
    }
    if (add_playlist_dialog_) {
        add_playlist_dialog_->show_tools();
    }
}

void MainWindow::on_core_events() {
    for (const core::CoreEvent& event : events_.take()) {
        std::visit([this](const auto& payload) { apply_event(payload); }, event);
    }
    if (std::exchange(playlists_outdated_, false)) {
        show_playlists();
    }
    tracks_.show_activities(sync_board_.activities());
    log_.show_added();
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

void MainWindow::apply_event(const core::PlaylistLookedUp& event) {
    if (add_playlist_dialog_) {
        add_playlist_dialog_->show_lookup(event);
    }
}

void MainWindow::apply_event(const core::SyncQueued& event) {
    sync_board_.apply(event);
    playlists_outdated_ = true;
}

void MainWindow::apply_event(const core::PlaylistFetching& event) {
    sync_board_.apply(event);
    playlists_outdated_ = true;
}

void MainWindow::apply_event(const core::PlaylistPageFetched&) {
    playlists_outdated_ = true;
}

void MainWindow::apply_event(const core::PlaylistFetched& event) {
    sync_board_.finish_playlist(event.playlist);
    playlists_outdated_ = true;
}

void MainWindow::apply_event(const core::PlaylistFetchFailed& event) {
    sync_board_.finish_playlist(event.playlist);
    playlists_outdated_ = true;
}

void MainWindow::apply_event(const core::TracksQueued& event) {
    sync_board_.apply(event);
    playlists_outdated_ = true;
}

void MainWindow::apply_event(const core::TrackMatching& event) {
    sync_board_.apply(event);
    playlists_outdated_ = true;
}

void MainWindow::apply_event(const core::DownloadsQueued& event) {
    sync_board_.apply(event);
    playlists_outdated_ = true;
}

void MainWindow::apply_event(const core::TrackDownloading& event) {
    sync_board_.apply(event);
    playlists_outdated_ = true;
}

// Only the track's ring changes, so the rows stay as they are
void MainWindow::apply_event(const core::TrackProgress& event) {
    sync_board_.apply(event);
}

void MainWindow::apply_event(const core::DownloadsWaiting& event) {
    sync_board_.apply(event);
    playlists_outdated_ = true;
}

void MainWindow::apply_event(const core::TrackStatusChanged& event) {
    sync_board_.finish_track(event.track);
    playlists_outdated_ = true;
}

void MainWindow::apply_event(const core::TracksMerged& event) {
    log_.rename_track(event.merged, event.survivor);
    sync_board_.finish_track(event.merged);
    playlists_outdated_ = true;
}

void MainWindow::apply_event(const core::SyncFinished&) {
    sync_board_.finish();
    playlists_outdated_ = true;
}

void MainWindow::apply_event(const core::CandidateSearchResults& results) {
    if (choose_match_dialog_) {
        choose_match_dialog_->show_results(results);
    }
}

void MainWindow::apply_event(const core::LogLine& line) {
    log_.add(line);
}

void MainWindow::on_command(WPARAM wparam, LPARAM lparam) {
    // Only menus and accelerators send WM_COMMAND without a control
    if (lparam == 0) {
        run_command(static_cast<CommandId>(LOWORD(wparam)));
    }
}

void MainWindow::on_activate(WORD state) {
    // Coming back, the window gets the focus itself, and WM_SETFOCUS hands it on to the child that had it
    if (state == WA_INACTIVE) {
        focus_.save(hwnd());
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
    // The workers wind down while the window goes, and whatever they still report stays in the queue
    tools_.stop();
    lookup_.stop();
    candidate_search_.stop();
    sync_.cancel();
    events_.set_target(nullptr);
    save_state();
    // A folder typed into the field takes effect when the field loses focus, which a close doesn't wait for
    welcome_.commit_library_folder();
    PostQuitMessage(0);
}

}
