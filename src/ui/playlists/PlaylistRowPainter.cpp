#include "ui/playlists/PlaylistRowPainter.h"

#include <algorithm>
#include <format>
#include <string>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/SourceLogo.h"
#include "ui/TrackCountText.h"

namespace westie::ui {

namespace {

constexpr int GlyphDips = 14;
// The logo sits in a box as wide as the All tracks glyph, so the names line up
constexpr int LogoBoxDips = 16;
constexpr int LogoGapDips = 6;
constexpr int SidePaddingDips = 7;
constexpr int TopPaddingDips = 5;
constexpr int BottomPaddingDips = 6;
constexpr int NameLineDips = 18;
constexpr int LineDips = 16;
// Name, track count, bar, and the last sync; failures and reviews add a line once tracks exist
constexpr int LinesUnderName = 3;
constexpr int BarHeightDips = 6;
constexpr int BarGapDips = 8;
constexpr int StatusGapDips = 6;
constexpr int SyncGapDips = 4;
constexpr int SyncButtonDips = 20;
// A hairline stays one pixel at every scale
constexpr int HairlinePixels = 1;
constexpr COLORREF BarColor = RGB(0xE6, 0xE6, 0xE6);
constexpr COLORREF ProgressColor = RGB(0x06, 0xB0, 0x25);
constexpr UINT LineFormat = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS;
constexpr std::wstring_view NeverSynced = L"Never synced";

int text_width(HDC dc, std::wstring_view text) {
    SIZE size{};
    platform::win32::check(GetTextExtentPoint32W(dc, text.data(), static_cast<int>(text.size()), &size), "GetTextExtentPoint32W");
    return size.cx;
}

void fill(HDC dc, const framework::Rect& rect, COLORREF color) {
    RECT area = rect.to_rect();
    SetDCBrushColor(dc, color);
    FillRect(dc, &area, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
}

}

PlaylistRowPainter::PlaylistRowPainter(framework::Dpi dpi, const framework::FontSet& fonts) :
        dpi_(dpi),
        youtube_music_logo_(source_logo_icon(core::SourceType::YouTubeMusic), dpi.scale(GlyphDips)),
        spotify_logo_(source_logo_icon(core::SourceType::Spotify), dpi.scale(GlyphDips)),
        waiting_(IDI_WAITING, dpi.scale(GlyphDips)),
        sync_(IDI_SYNC_GRAY, dpi.scale(GlyphDips)),
        name_font_(fonts.semibold().handle()),
        line_font_(fonts.regular().handle()),
        name_line_height_(std::max(dpi.scale(NameLineDips), fonts.semibold().line_height())),
        line_height_(std::max(dpi.scale(LineDips), fonts.regular().line_height())) {}

int PlaylistRowPainter::height() const noexcept {
    return 2 * HairlinePixels + dpi_.scale(TopPaddingDips) + name_line_height_ + LinesUnderName * line_height_ + dpi_.scale(BottomPaddingDips);
}

int PlaylistRowPainter::text_left() const noexcept {
    return HairlinePixels + dpi_.scale(SidePaddingDips) + dpi_.scale(LogoBoxDips) + dpi_.scale(LogoGapDips);
}

void PlaylistRowPainter::paint(HDC dc, const framework::Rect& bounds, const PlaylistRow& row, RowLook look) const {
    paint_row_look(dc, bounds, look);
    SetBkMode(dc, TRANSPARENT);
    framework::Rect content{
            bounds.left + HairlinePixels + dpi_.scale(SidePaddingDips), bounds.top + HairlinePixels + dpi_.scale(TopPaddingDips),
            bounds.right - HairlinePixels - dpi_.scale(SidePaddingDips), bounds.bottom
    };
    paint_name(dc, content.take_top(name_line_height_), row);

    // The lines under the name start where the name does
    content.left = bounds.left + text_left();
    SelectObject(dc, line_font_);
    SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
    draw_line(dc, track_count_text(row.counts.total), content.take_top(line_height_));
    paint_bar(dc, content.take_top(line_height_), row);
    paint_last_sync(dc, content.take_top(line_height_));
}

const framework::IconResource& PlaylistRowPainter::logo(core::SourceType type) const noexcept {
    return type == core::SourceType::Spotify ? spotify_logo_ : youtube_music_logo_;
}

void PlaylistRowPainter::draw_line(HDC dc, std::wstring_view text, framework::Rect line) const {
    RECT area = line.to_rect();
    DrawTextW(dc, text.data(), static_cast<int>(text.size()), &area, LineFormat);
}

void PlaylistRowPainter::paint_name(HDC dc, framework::Rect line, const PlaylistRow& row) const {
    const framework::IconResource& icon = logo(row.type);
    framework::Rect box = line.take_left(dpi_.scale(LogoBoxDips) + dpi_.scale(LogoGapDips));
    icon.draw(dc, box.left + (dpi_.scale(LogoBoxDips) - icon.size()) / 2, line.top + (line.height() - icon.size()) / 2);

    SelectObject(dc, name_font_);
    SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
    draw_line(dc, row.name, line);
}

void PlaylistRowPainter::paint_bar(HDC dc, framework::Rect line, const PlaylistRow& row) const {
    std::wstring counts = std::format(L"{}/{}", row.counts.synced, row.counts.total);
    draw_line(dc, counts, line.take_right(text_width(dc, counts)));
    line.take_right(dpi_.scale(BarGapDips));

    int bar_top = line.top + (line.height() - dpi_.scale(BarHeightDips)) / 2;
    framework::Rect bar{line.left, bar_top, line.right, bar_top + dpi_.scale(BarHeightDips)};
    fill(dc, bar, BarColor);
    if (row.counts.total > 0) {
        fill(dc, bar.take_left(bar.width() * row.counts.synced / row.counts.total), ProgressColor);
    }
}

void PlaylistRowPainter::paint_last_sync(HDC dc, framework::Rect line) const {
    // The status icon is flush with the bar above
    waiting_.draw(dc, line.left, line.top + (line.height() - waiting_.size()) / 2);
    line.take_left(waiting_.size() + dpi_.scale(StatusGapDips));

    // The sync button follows the text, and the text gives way to keep it in the row
    int room = std::max(0, line.width() - dpi_.scale(SyncGapDips) - dpi_.scale(SyncButtonDips));
    framework::Rect text = line.take_left(std::min(text_width(dc, NeverSynced), room));
    draw_line(dc, NeverSynced, text);

    int button_size = dpi_.scale(SyncButtonDips);
    int button_left = text.right + dpi_.scale(SyncGapDips);
    sync_.draw(dc, button_left + (button_size - sync_.size()) / 2, line.top + (line.height() - sync_.size()) / 2);
}

}
