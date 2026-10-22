#pragma once

#include <functional>
#include <optional>
#include <vector>

#include <windows.h>

#include "ui/commands/CommandId.h"
#include "ui/framework/Button.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Label.h"
#include "ui/framework/Rect.h"
#include "ui/framework/Window.h"
#include "ui/playlists/PlaylistRow.h"
#include "ui/playlists/PlaylistRowPainter.h"

namespace westie::ui {

class PlaylistList final : public framework::Window {
public:
    PlaylistList(HWND parent, std::function<void(CommandId)> on_command);

    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void set_rows(std::vector<PlaylistRow> rows);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    framework::Rect client() const;
    framework::Rect row_bounds(std::size_t index) const;
    int rows_bottom() const;
    int content_height() const;
    void layout();
    void scroll_to(int position);
    void paint() const;
    void on_vertical_scroll(WORD request);
    void on_mouse_wheel(short delta);
    void on_command(WPARAM wparam) const;

    std::function<void(CommandId)> on_command_;
    framework::Label empty_text_;
    framework::Button add_button_;
    std::optional<PlaylistRowPainter> painter_;
    std::vector<PlaylistRow> rows_;
    int empty_text_height_ = 0;
    int scroll_ = 0;
    int wheel_remainder_ = 0;
};

}
