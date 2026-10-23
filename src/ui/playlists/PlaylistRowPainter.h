#pragma once

#include <chrono>
#include <string_view>
#include <vector>

#include <windows.h>

#include "core/sources/SourceType.h"
#include "ui/RowLook.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/Rect.h"
#include "ui/playlists/LastSyncLine.h"
#include "ui/playlists/PlaylistRow.h"

namespace westie::ui {

struct RowPaint {
    RowLook look = RowLook::Normal;
    bool sync_hot = false;
    // Which of the spinning control's steps shows
    int spin_step = 0;
    std::chrono::system_clock::time_point now{};
};

class PlaylistRowPainter {
public:
    constexpr static int SpinSteps = 12;

    PlaylistRowPainter(framework::Dpi dpi, const framework::FontSet& fonts);

    int height(const PlaylistRow& row) const noexcept;
    int text_left() const noexcept;
    framework::Rect sync_button(const framework::Rect& bounds, const PlaylistRow& row, std::chrono::system_clock::time_point now) const;
    void paint(HDC dc, const framework::Rect& bounds, const PlaylistRow& row, const RowPaint& look) const;

private:
    static bool has_counts_line(const PlaylistRow& row) noexcept;
    framework::Rect content(const framework::Rect& bounds) const noexcept;
    framework::Rect last_line(const framework::Rect& bounds, const PlaylistRow& row) const noexcept;
    framework::Rect place_button(HDC dc, framework::Rect line, const LastSyncLine& sync) const;
    const framework::IconResource& logo(core::SourceType type) const noexcept;
    const framework::IconResource* mark_icon(SyncMark mark) const noexcept;
    const framework::IconResource& control_icon(SyncControl control, int spin_step) const noexcept;
    void draw_line(HDC dc, std::wstring_view text, framework::Rect line) const;
    void paint_name(HDC dc, framework::Rect line, const PlaylistRow& row) const;
    void paint_bar(HDC dc, framework::Rect line, const PlaylistRow& row) const;
    void paint_counts(HDC dc, framework::Rect line, const PlaylistRow& row) const;
    void paint_last_sync(HDC dc, framework::Rect line, const PlaylistRow& row, const RowPaint& look) const;

    framework::Dpi dpi_;
    framework::IconResource youtube_music_logo_;
    framework::IconResource spotify_logo_;
    framework::IconResource synced_;
    framework::IconResource waiting_;
    framework::IconResource failed_;
    framework::IconResource warn_;
    framework::IconResource tool_wait_;
    framework::IconResource sync_;
    framework::IconResource queued_;
    std::vector<framework::IconResource> spin_;
    HFONT name_font_ = nullptr;
    HFONT line_font_ = nullptr;
    int name_line_height_ = 0;
    int line_height_ = 0;
};

}
