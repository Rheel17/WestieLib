#pragma once

#include <windows.h>

#include "ui/commands/CommandId.h"
#include "ui/commands/CommandState.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/MessageFont.h"
#include "ui/framework/Splitter.h"
#include "ui/framework/Window.h"
#include "ui/framework/WindowIcon.h"
#include "ui/inspector/InspectorPanel.h"
#include "ui/main/MainLayout.h"
#include "ui/main/WindowState.h"
#include "ui/playlists/PlaylistPanel.h"
#include "ui/statusbar/MainStatusBar.h"
#include "ui/toolbar/MainToolbar.h"
#include "ui/tracks/TrackPanel.h"

namespace westie::ui {

class MainWindow final : public framework::Window {
public:
    MainWindow();

    void show(int show_command) override;

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void attach_menu();
    void apply_dpi(framework::Dpi dpi);
    void apply_initial_size();
    void restore_placement(const WindowState& state);
    void save_state() const;
    MainLayout current_layout() const;
    void layout_panels();
    CommandState command_state() const;
    void run_command(CommandId command);
    void on_command(WPARAM wparam, LPARAM lparam);
    void on_init_menu_popup(HMENU menu, bool window_menu) const;
    LRESULT on_draw_item(const DRAWITEMSTRUCT& item) const;
    void on_playlists_splitter_drag(int left);
    void on_inspector_splitter_drag(int left);
    void on_get_min_max_info(MINMAXINFO& info) const;
    void on_dpi_changed(UINT dpi, const RECT& suggested);
    void on_setting_change(WPARAM setting);
    void on_destroy();

    framework::WindowIcon icon_;
    framework::MessageFont font_;
    MainToolbar toolbar_;
    PlaylistPanel playlists_;
    framework::Splitter playlists_splitter_;
    TrackPanel tracks_;
    framework::Splitter inspector_splitter_;
    InspectorPanel inspector_;
    MainStatusBar status_bar_;
    PanelWidths widths_;
    bool start_maximized_ = false;
    bool restoring_placement_ = false;
};

}
