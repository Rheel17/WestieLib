#include "ui/SyncBar.h"

namespace westie::ui {

namespace {

constexpr COLORREF BarColor = RGB(0xE6, 0xE6, 0xE6);
constexpr COLORREF ProgressColor = RGB(0x06, 0xB0, 0x25);

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

}
