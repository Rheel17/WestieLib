#include "ui/playlists/PlaylistRowPainter.h"

#include <algorithm>
#include <format>
#include <string>
#include <utility>

#include "ResourceIds.h"
#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"
#include "ui/SourceLogo.h"
#include "ui/SyncBar.h"
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
// Track count, bar and last sync; the counts that need attention add a line between the last two
constexpr int LinesUnderName = 3;
constexpr int BarHeightDips = 6;
constexpr int BarGapDips = 8;
constexpr int StatusGapDips = 6;
constexpr int SyncGapDips = 4;
constexpr int SyncButtonDips = 20;
// A hairline stays one pixel at every scale
constexpr int HairlinePixels = 1;
constexpr UINT LineFormat = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS;
constexpr std::wstring_view CountSeparator = L" · ";
constexpr COLORREF FailedColor = RGB(0xC4, 0x2B, 0x1C);
constexpr COLORREF WarningColor = RGB(0x9D, 0x5D, 0x00);
constexpr COLORREF HotFill = RGB(0xE5, 0xF3, 0xFF);
constexpr COLORREF HotBorder = RGB(0xCC, 0xE8, 0xFF);
constexpr COLORREF SelectedHotBorder = RGB(0x99, 0xD1, 0xFF);

struct CountPart {
    std::wstring text{};
    COLORREF color = 0;
};

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

std::vector<framework::IconResource> load_spin(int size) {
    std::vector<framework::IconResource> frames;
    for (int step = 0; step < PlaylistRowPainter::SpinSteps; ++step) {
        frames.emplace_back(IDI_SPIN_0 + step, size);
    }
    return frames;
}

}

PlaylistRowPainter::PlaylistRowPainter(framework::Dpi dpi, const framework::FontSet& fonts) :
        dpi_(dpi),
        youtube_music_logo_(source_logo_icon(core::SourceType::YouTubeMusic), dpi.scale(GlyphDips)),
        spotify_logo_(source_logo_icon(core::SourceType::Spotify), dpi.scale(GlyphDips)),
        synced_(IDI_SYNCED, dpi.scale(GlyphDips)),
        waiting_(IDI_WAITING, dpi.scale(GlyphDips)),
        failed_(IDI_FAILED, dpi.scale(GlyphDips)),
        warn_(IDI_WARN, dpi.scale(GlyphDips)),
        tool_wait_(IDI_TOOL_WAIT, dpi.scale(GlyphDips)),
        sync_(IDI_SYNC_GRAY, dpi.scale(GlyphDips)),
        queued_(IDI_SYNCING_FADED, dpi.scale(GlyphDips)),
        spin_(load_spin(dpi.scale(GlyphDips))),
        name_font_(fonts.semibold().handle()),
        line_font_(fonts.regular().handle()),
        name_line_height_(std::max(dpi.scale(NameLineDips), fonts.semibold().line_height())),
        line_height_(std::max(dpi.scale(LineDips), fonts.regular().line_height())) {}

int PlaylistRowPainter::height(const PlaylistRow& row) const noexcept {
    int lines = LinesUnderName + (has_counts_line(row) ? 1 : 0);
    return 2 * HairlinePixels + dpi_.scale(TopPaddingDips) + name_line_height_ + lines * line_height_ + dpi_.scale(BottomPaddingDips);
}

int PlaylistRowPainter::text_left() const noexcept {
    return HairlinePixels + dpi_.scale(SidePaddingDips) + dpi_.scale(LogoBoxDips) + dpi_.scale(LogoGapDips);
}

framework::Rect PlaylistRowPainter::sync_button(const framework::Rect& bounds, const PlaylistRow& row, std::chrono::system_clock::time_point now) const {
    platform::win32::UniqueMemoryDc dc(platform::win32::check(CreateCompatibleDC(nullptr), "CreateCompatibleDC"));
    SelectObject(dc.get(), line_font_);
    return place_button(dc.get(), last_line(bounds, row), last_sync_line(row, now));
}

void PlaylistRowPainter::paint(HDC dc, const framework::Rect& bounds, const PlaylistRow& row, const RowPaint& look) const {
    paint_row_look(dc, bounds, look.look);
    SetBkMode(dc, TRANSPARENT);
    framework::Rect area = content(bounds);
    paint_name(dc, area.take_top(name_line_height_), row);

    // The lines under the name start where the name does
    area.left = bounds.left + text_left();
    SelectObject(dc, line_font_);
    SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
    draw_line(dc, track_count_text(row.counts.total), area.take_top(line_height_));
    paint_bar(dc, area.take_top(line_height_), row);
    if (has_counts_line(row)) {
        paint_counts(dc, area.take_top(line_height_), row);
    }
    paint_last_sync(dc, area.take_top(line_height_), row, look);
}

bool PlaylistRowPainter::has_counts_line(const PlaylistRow& row) noexcept {
    return row.counts.waiting > 0 || row.counts.failed > 0 || row.counts.to_review > 0;
}

framework::Rect PlaylistRowPainter::content(const framework::Rect& bounds) const noexcept {
    return {
        bounds.left + HairlinePixels + dpi_.scale(SidePaddingDips), bounds.top + HairlinePixels + dpi_.scale(TopPaddingDips),
        bounds.right - HairlinePixels - dpi_.scale(SidePaddingDips), bounds.bottom
    };
}

framework::Rect PlaylistRowPainter::last_line(const framework::Rect& bounds, const PlaylistRow& row) const noexcept {
    framework::Rect area = content(bounds);
    area.take_top(name_line_height_ + (LinesUnderName - 1 + (has_counts_line(row) ? 1 : 0)) * line_height_);
    area.left = bounds.left + text_left();
    return area.take_top(line_height_);
}

framework::Rect PlaylistRowPainter::place_button(HDC dc, framework::Rect line, const LastSyncLine& sync) const {
    if (mark_icon(sync.mark)) {
        line.take_left(dpi_.scale(GlyphDips) + dpi_.scale(StatusGapDips));
    }
    // The button follows the text, and the text gives way to keep it in the row
    int room = std::max(0, line.width() - dpi_.scale(SyncGapDips) - dpi_.scale(SyncButtonDips));
    framework::Rect text = line.take_left(std::min(text_width(dc, sync.warning) + text_width(dc, sync.text), room));
    int size = dpi_.scale(SyncButtonDips);
    int left = text.right + dpi_.scale(SyncGapDips);
    int top = line.top + (line.height() - size) / 2;
    return {left, top, left + size, top + size};
}

const framework::IconResource& PlaylistRowPainter::logo(core::SourceType type) const noexcept {
    return type == core::SourceType::Spotify ? spotify_logo_ : youtube_music_logo_;
}

const framework::IconResource* PlaylistRowPainter::mark_icon(SyncMark mark) const noexcept {
    switch (mark) {
    case SyncMark::FetchFailed:     return &warn_;
    case SyncMark::Failures:        return &failed_;
    case SyncMark::WaitingForTools: return &tool_wait_;
    case SyncMark::Unsynced:        return &waiting_;
    case SyncMark::Synced:          return &synced_;
    case SyncMark::None:            return nullptr;
    }
    return nullptr;
}

const framework::IconResource& PlaylistRowPainter::control_icon(SyncControl control, int spin_step) const noexcept {
    switch (control) {
    case SyncControl::Spinning: return spin_[static_cast<std::size_t>(spin_step % SpinSteps)];
    case SyncControl::Queued:   return queued_;
    case SyncControl::Idle:     return sync_;
    }
    return sync_;
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
    paint_sync_bar(dc, {line.left, bar_top, line.right, bar_top + dpi_.scale(BarHeightDips)}, row.counts);
}

void PlaylistRowPainter::paint_counts(HDC dc, framework::Rect line, const PlaylistRow& row) const {
    std::vector<CountPart> parts;
    COLORREF gray = GetSysColor(COLOR_GRAYTEXT);
    if (row.counts.waiting > 0) {
        parts.push_back({std::format(L"{} waiting", row.counts.waiting), gray});
    }
    if (row.counts.failed > 0) {
        parts.push_back({std::format(L"{} failed", row.counts.failed), FailedColor});
    }
    if (row.counts.to_review > 0) {
        parts.push_back({std::format(L"{} to review", row.counts.to_review), WarningColor});
    }
    for (std::size_t index = 0; index < parts.size() && line.width() > 0; ++index) {
        if (index > 0) {
            SetTextColor(dc, gray);
            draw_line(dc, CountSeparator, line);
            line.take_left(std::min(line.width(), text_width(dc, CountSeparator)));
        }
        SetTextColor(dc, parts[index].color);
        draw_line(dc, parts[index].text, line);
        line.take_left(std::min(line.width(), text_width(dc, parts[index].text)));
    }
    SetTextColor(dc, gray);
}

void PlaylistRowPainter::paint_last_sync(HDC dc, framework::Rect line, const PlaylistRow& row, const RowPaint& look) const {
    LastSyncLine sync = last_sync_line(row, look.now);
    framework::Rect button = place_button(dc, line, sync);

    // The status icon is flush with the bar above
    if (const framework::IconResource* icon = mark_icon(sync.mark)) {
        icon->draw(dc, line.left, line.top + (line.height() - icon->size()) / 2);
        line.take_left(icon->size() + dpi_.scale(StatusGapDips));
    }
    framework::Rect text{line.left, line.top, std::max(line.left, button.left - dpi_.scale(SyncGapDips)), line.bottom};
    if (!sync.warning.empty()) {
        SetTextColor(dc, WarningColor);
        draw_line(dc, sync.warning, text);
        text.take_left(std::min(text.width(), text_width(dc, sync.warning)));
        SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
    }
    draw_line(dc, sync.text, text);

    if (look.sync_hot && sync.control == SyncControl::Idle) {
        fill(dc, button, look.look == RowLook::Selected ? SelectedHotBorder : HotBorder);
        framework::Rect inside = button;
        inside.take_left(HairlinePixels);
        inside.take_right(HairlinePixels);
        inside.take_top(HairlinePixels);
        inside.take_bottom(HairlinePixels);
        fill(dc, inside, HotFill);
    }
    const framework::IconResource& icon = control_icon(sync.control, look.spin_step);
    icon.draw(dc, button.left + (button.width() - icon.size()) / 2, button.top + (button.height() - icon.size()) / 2);
}

}
