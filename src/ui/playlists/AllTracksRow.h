#pragma once

#include <cstddef>
#include <functional>
#include <optional>

#include <windows.h>

#include "ui/TrackCounts.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/ItemTracker.h"
#include "ui/framework/Window.h"

namespace westie::ui {

class AllTracksRow final : public framework::Window {
public:
    AllTracksRow(HWND parent, std::function<void()> on_click);

    int height() const noexcept;
    int text_left() const;
    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void set_counts(const TrackCounts& counts);
    void set_selected(bool selected);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    std::optional<std::size_t> item_at(LPARAM lparam) const;
    void repaint_if(bool changed) const;
    void on_left_button_up(LPARAM lparam);
    void paint() const;

    std::function<void()> on_click_;
    framework::ItemTracker mouse_;
    framework::IconResource glyph_;
    HFONT name_font_ = nullptr;
    HFONT counts_font_ = nullptr;
    int name_line_height_ = 0;
    int counts_line_height_ = 0;
    int height_ = 0;
    TrackCounts counts_;
    bool selected_ = false;
};

}
