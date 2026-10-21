#include "ui/playlists/AllTracksRow.h"

#include <algorithm>
#include <format>
#include <string>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/TrackCountText.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui {

namespace {

constexpr int GlyphDips = 16;
constexpr int GlyphGapDips = 6;
constexpr int SidePaddingDips = 7;
constexpr int TopPaddingDips = 5;
constexpr int BottomPaddingDips = 6;
constexpr int NameLineDips = 18;
constexpr int CountsLineDips = 16;
// A hairline stays one pixel at every scale
constexpr int HairlinePixels = 1;
constexpr COLORREF SelectionColor = RGB(0xCC, 0xE8, 0xFF);
constexpr COLORREF SelectionBorderColor = RGB(0x99, 0xD1, 0xFF);
constexpr UINT LineFormat = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS;
constexpr wchar_t Name[] = L"All tracks";

}

AllTracksRow::AllTracksRow(HWND parent) :
        Window(window_class(), {.title = Name, .style = WS_CHILD | WS_VISIBLE, .parent = parent}),
        glyph_(IDI_LIBRARY, framework::Dpi::of(hwnd()).scale(GlyphDips)) {}

int AllTracksRow::height() const noexcept {
    return height_;
}

int AllTracksRow::text_left() const {
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    return HairlinePixels + dpi.scale(SidePaddingDips) + glyph_.size() + dpi.scale(GlyphGapDips);
}

void AllTracksRow::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    glyph_ = framework::IconResource(IDI_LIBRARY, dpi.scale(GlyphDips));
    name_font_ = fonts.semibold().handle();
    counts_font_ = fonts.regular().handle();
    name_line_height_ = std::max(dpi.scale(NameLineDips), fonts.semibold().line_height());
    counts_line_height_ = std::max(dpi.scale(CountsLineDips), fonts.regular().line_height());
    height_ = 2 * HairlinePixels + dpi.scale(TopPaddingDips) + name_line_height_ + counts_line_height_ + dpi.scale(BottomPaddingDips);
    platform::win32::check(InvalidateRect(hwnd(), nullptr, TRUE), "InvalidateRect");
}

void AllTracksRow::set_counts(const TrackCounts& counts) {
    counts_ = counts;
    platform::win32::check(InvalidateRect(hwnd(), nullptr, TRUE), "InvalidateRect");
}

LRESULT AllTracksRow::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_PAINT: paint(); return 0;
    default:       return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& AllTracksRow::window_class() {
    static framework::WindowClass window_class({.name = L"WestieLib.AllTracksRow", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

void AllTracksRow::paint() const {
    framework::BufferedPaint paint(hwnd());
    HDC dc = paint.dc();
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::Dpi dpi = framework::Dpi::of(hwnd());

    // Selected, since nothing else can be selected yet
    auto brush = static_cast<HBRUSH>(GetStockObject(DC_BRUSH));
    SetDCBrushColor(dc, SelectionColor);
    FillRect(dc, &client, brush);
    SetDCBrushColor(dc, SelectionBorderColor);
    FrameRect(dc, &client, brush);

    int left = HairlinePixels + dpi.scale(SidePaddingDips);
    int right = client.right - HairlinePixels - dpi.scale(SidePaddingDips);
    int top = HairlinePixels + dpi.scale(TopPaddingDips);
    glyph_.draw(dc, left, top + (name_line_height_ - glyph_.size()) / 2);

    SetBkMode(dc, TRANSPARENT);
    RECT name_line{text_left(), top, right, top + name_line_height_};
    SelectObject(dc, name_font_);
    SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
    DrawTextW(dc, Name, -1, &name_line, LineFormat);

    RECT counts_line{name_line.left, name_line.bottom, right, name_line.bottom + counts_line_height_};
    std::wstring counts = std::format(L"{} · {} synced", track_count_text(counts_.total), counts_.synced);
    SelectObject(dc, counts_font_);
    SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
    DrawTextW(dc, counts.c_str(), static_cast<int>(counts.size()), &counts_line, LineFormat);
}

}
