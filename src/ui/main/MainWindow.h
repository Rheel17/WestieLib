#pragma once

#include <filesystem>

#include <windows.h>

#include "core/Settings.h"
#include "core/SettingsStore.h"
#include "ui/TrackCounts.h"
#include "ui/commands/CommandId.h"
#include "ui/commands/CommandState.h"
#include "ui/framework/Dpi.h"
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
#include "ui/statusbar/MainStatusBar.h"
#include "ui/toolbar/MainToolbar.h"
#include "ui/tracks/TrackPanel.h"
#include "ui/welcome/WelcomePanel.h"

namespace westie::ui {

class MainWindow final : public framework::Window {
public:
    MainWindow(core::SettingsStore& settings_store, core::Settings settings);

    void show(int show_command) override;

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void attach_menu();
    void apply_dpi(framework::Dpi dpi);
    void show_track_counts(const TrackCounts& counts);
    void change_library_folder(const std::filesystem::path& folder);
    void save_state() const;
    MainLayout current_layout() const;
    void layout_panels();
    CommandState command_state() const;
    void update_commands();
    void run_command(CommandId command);
    void toggle_log();
    void on_command(WPARAM wparam, LPARAM lparam);
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
};

}
