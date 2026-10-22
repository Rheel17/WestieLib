#pragma once

#include <string_view>

#include <windows.h>

#include "core/sources/SourceType.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/Rect.h"
#include "ui/playlists/PlaylistRow.h"

namespace westie::ui {

class PlaylistRowPainter {
public:
    PlaylistRowPainter(framework::Dpi dpi, const framework::FontSet& fonts);

    int height() const noexcept;
    int text_left() const noexcept;
    void paint(HDC dc, const framework::Rect& bounds, const PlaylistRow& row) const;

private:
    const framework::IconResource& logo(core::SourceType type) const noexcept;
    void draw_line(HDC dc, std::wstring_view text, framework::Rect line) const;
    void paint_name(HDC dc, framework::Rect line, const PlaylistRow& row) const;
    void paint_bar(HDC dc, framework::Rect line, const PlaylistRow& row) const;
    void paint_last_sync(HDC dc, framework::Rect line) const;

    framework::Dpi dpi_;
    framework::IconResource youtube_music_logo_;
    framework::IconResource spotify_logo_;
    framework::IconResource waiting_;
    framework::IconResource sync_;
    HFONT name_font_ = nullptr;
    HFONT line_font_ = nullptr;
    int name_line_height_ = 0;
    int line_height_ = 0;
};

}
