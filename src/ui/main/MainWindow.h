#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <windows.h>

#include "core/playlists/PlaylistLink.h"
#include "core/playlists/PlaylistLookedUp.h"
#include "core/playlists/PlaylistLookup.h"
#include "core/settings/Settings.h"
#include "core/settings/SettingsStore.h"
#include "core/state/StateStore.h"
#include "core/tools/ToolManager.h"
#include "core/tools/ToolStatus.h"
#include "core/tools/ToolsChecked.h"
#include "core/tools/ToolsRunChanged.h"
#include "ui/TrackCounts.h"
#include "ui/commands/CommandId.h"
#include "ui/commands/CommandState.h"
#include "ui/dialogs/AddPlaylistDialog.h"
#include "ui/dialogs/SettingsDialog.h"
#include "ui/events/CoreEventQueue.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FocusMemory.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/MenuBarPainter.h"
#include "ui/framework/Splitter.h"
#include "ui/framework/Window.h"
#include "ui/framework/WindowIcon.h"
#include "ui/inspector/InspectorPanel.h"
#include "ui/log/LogPanel.h"
#include "ui/main/MainLayout.h"
#include "ui/main/WindowPlacement.h"
#include "ui/playlists/PlaylistPanel.h"
#include "ui/playlists/PlaylistRow.h"
#include "ui/statusbar/MainStatusBar.h"
#include "ui/toolbar/MainToolbar.h"
#include "ui/tools/ToolBoard.h"
#include "ui/tracks/TrackPanel.h"
#include "ui/welcome/WelcomePanel.h"

namespace westie::ui {

class MainWindow final : public framework::Window {
public:
    MainWindow(
            core::SettingsStore& settings_store,
            core::Settings settings,
            core::StateStore& state,
            CoreEventQueue& events,
            core::ToolManager& tools,
            core::PlaylistLookup& lookup
    );

    void show(int show_command) override;

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void attach_menu();
    void apply_dpi(framework::Dpi dpi);
    void show_track_counts(const TrackCounts& counts);
    void change_library_folder(const std::filesystem::path& folder);
    bool has_playlists() const;
    std::optional<std::string> added_playlist_name(const core::PlaylistLink& link) const;
    void add_playlist();
    void select_playlist(std::optional<std::string> key);
    void show_scope();
    void show_inspector();
    void show_playlists();
    void save_state() const;
    MainLayout current_layout() const;
    void layout_panels();
    CommandState command_state() const;
    void update_commands();
    void run_command(CommandId command);
    std::optional<PlaylistRow> selected_row() const;
    void show_playlist_menu(POINT screen);
    void remove_playlist();
    void open_library_folder() const;
    void open_playlist_page() const;
    void copy_playlist_link() const;
    void toggle_log();
    void show_settings();
    void show_tools();
    void on_core_events();
    void apply_event(const core::ToolsRunChanged& event);
    void apply_event(const core::ToolStatus& status);
    void apply_event(const core::ToolsChecked& event);
    void apply_event(const core::PlaylistLookedUp& event);
    void on_command(WPARAM wparam, LPARAM lparam);
    void on_activate(WORD state);
    void on_init_menu_popup(HMENU menu, bool window_menu) const;
    LRESULT on_draw_item(const DRAWITEMSTRUCT& item) const;
    void on_playlists_splitter_drag(int left);
    void on_inspector_splitter_drag(int left);
    void on_log_splitter_drag(int top);
    void on_get_min_max_info(MINMAXINFO& info) const;
    void on_dpi_changed(UINT dpi, const RECT& suggested);
    void on_setting_change(WPARAM setting);
    LRESULT on_non_client_paint(UINT message, WPARAM wparam, LPARAM lparam);
    void on_destroy();

    core::SettingsStore& settings_store_;
    core::Settings settings_;
    core::StateStore& state_;
    CoreEventQueue& events_;
    core::ToolManager& tools_;
    core::PlaylistLookup& lookup_;
    ToolBoard tool_board_;
    WindowPlacement placement_;
    framework::WindowIcon icon_;
    framework::MenuBarPainter menu_bar_;
    framework::FontSet fonts_;
    MainToolbar toolbar_;
    PlaylistPanel playlists_;
    framework::Splitter playlists_splitter_;
    TrackPanel tracks_;
    WelcomePanel welcome_;
    framework::Splitter inspector_splitter_;
    InspectorPanel inspector_;
    framework::Splitter log_splitter_;
    LogPanel log_;
    MainStatusBar status_bar_;
    PanelState panels_;
    framework::FocusMemory focus_;
    TrackCounts track_counts_;
    // A playlist's key, or nothing while All tracks is selected
    std::optional<std::string> selected_playlist_;
    // Only while they're open, so tool and look-up events can reach them
    std::optional<SettingsDialog> settings_dialog_;
    std::optional<AddPlaylistDialog> add_playlist_dialog_;
};

}
