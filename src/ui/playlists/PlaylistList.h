#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <windows.h>

#include "ui/framework/Animation.h"
#include "ui/framework/Button.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/ItemTracker.h"
#include "ui/framework/Label.h"
#include "ui/framework/Rect.h"
#include "ui/framework/Tooltip.h"
#include "ui/framework/VerticalScroll.h"
#include "ui/framework/Window.h"
#include "ui/playlists/PlaylistEvents.h"
#include "ui/playlists/PlaylistRow.h"
#include "ui/playlists/PlaylistRowPainter.h"

namespace westie::ui {

class PlaylistList final : public framework::Window {
public:
    PlaylistList(HWND parent, PlaylistEvents events);

    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void set_rows(std::vector<PlaylistRow> rows);
    void set_selection(const std::optional<std::string>& key);
    void set_sync_enabled(bool enabled);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    framework::Rect client() const;
    framework::Rect row_bounds(std::size_t index) const;
    int rows_bottom() const;
    int content_height() const;
    std::optional<std::size_t> selected_index() const;
    static POINT point_of(LPARAM lparam);
    std::optional<std::size_t> row_at(POINT point) const;
    std::optional<std::size_t> sync_at(POINT point) const;
    void layout();
    void place_tips();
    void reveal_selection();
    void scrolled(bool moved);
    void follow_cursor();
    void track_mouse(std::optional<POINT> point);
    void repaint_if(bool changed) const;
    void paint() const;
    void on_mouse_leave();
    void on_left_button_down(POINT point);
    void on_left_button_up(POINT point);
    void on_capture_changed();
    void on_timer(WPARAM timer) const;
    bool on_key_down(WPARAM key);
    void on_context_menu(POINT screen);
    void on_command(WPARAM wparam) const;

    PlaylistEvents events_;
    framework::Label empty_text_;
    framework::Button add_button_;
    framework::Tooltip tips_;
    std::optional<PlaylistRowPainter> painter_;
    std::vector<PlaylistRow> rows_;
    std::optional<std::string> selected_;
    framework::VerticalScroll scroll_;
    framework::ItemTracker mouse_;
    bool sync_enabled_ = false;
    std::optional<std::size_t> sync_hot_;
    std::optional<std::size_t> sync_pressed_;
    framework::Animation spin_;
    std::size_t tip_count_ = 0;
    int empty_text_height_ = 0;
};

}
