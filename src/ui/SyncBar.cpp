#include "ui/SyncBar.h"

#include <algorithm>
#include <cmath>

namespace westie::ui {

namespace {

constexpr COLORREF BarColor = RGB(0xE6, 0xE6, 0xE6);
constexpr COLORREF ProgressColor = RGB(0x06, 0xB0, 0x25);
constexpr COLORREF PausedColor = RGB(0xD9, 0xA4, 0x00);

void fill(HDC dc, const framework::Rect& rect, COLORREF color) {
    RECT area = rect.to_rect();
    SetDCBrushColor(dc, color);
    FillRect(dc, &area, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
}

}

void paint_sync_bar(HDC dc, framework::Rect bar, const TrackCounts& counts) {
    fill(dc, bar, BarColor);
    if (counts.total > 0) {
        fill(dc, bar.take_left(bar.width() * counts.synced / counts.total), ProgressColor);
    }
}

void paint_fraction_bar(HDC dc, framework::Rect bar, double fraction, BarFill bar_fill) {
    fill(dc, bar, BarColor);
    COLORREF color = bar_fill == BarFill::Paused ? PausedColor : ProgressColor;
    fill(dc, bar.take_left(static_cast<int>(std::lround(bar.width() * std::clamp(fraction, 0.0, 1.0)))), color);
}

}
