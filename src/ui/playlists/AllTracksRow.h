#pragma once

#include <windows.h>

#include "ui/TrackCounts.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/Window.h"

namespace westie::ui {

class AllTracksRow final : public framework::Window {
public:
    explicit AllTracksRow(HWND parent);

    int height() const noexcept;
    int text_left() const;
    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void set_counts(const TrackCounts& counts);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void paint() const;

    framework::IconResource glyph_;
    HFONT name_font_ = nullptr;
    HFONT counts_font_ = nullptr;
    int name_line_height_ = 0;
    int counts_line_height_ = 0;
    int height_ = 0;
    TrackCounts counts_;
};

}
