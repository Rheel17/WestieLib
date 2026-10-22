#include "ui/dialogs/LinkLinePainter.h"

#include <algorithm>
#include <string_view>

#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/SavedDc.h"
#include "ui/framework/Spinner.h"

namespace westie::ui {

namespace {

constexpr int IconDips = 16;
constexpr int IconGapDips = 6;
constexpr int SourceGapDips = 6;
constexpr UINT LineFormat = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS;

int text_width(HDC dc, std::wstring_view text) {
    SIZE size{};
    platform::win32::check(GetTextExtentPoint32W(dc, text.data(), static_cast<int>(text.size()), &size), "GetTextExtentPoint32W");
    return size.cx;
}

void draw_text(HDC dc, std::wstring_view text, RECT area) {
    DrawTextW(dc, text.data(), static_cast<int>(text.size()), &area, LineFormat);
}

void draw_line(HDC dc, HWND control, RECT area, const LinkLine& line, double spinner_turn) {
    framework::SavedDc saved(dc);
    FillRect(dc, &area, GetSysColorBrush(COLOR_BTNFACE));

    framework::Dpi dpi = framework::Dpi::of(control);
    RECT text = area;
    int icon_size = dpi.scale(IconDips);
    int icon_top = text.top + (text.bottom - text.top - icon_size) / 2;
    if (line.spinner) {
        framework::paint_spinner(dc, text.left, icon_top, icon_size, spinner_turn);
        text.left += icon_size + dpi.scale(IconGapDips);
    } else if (line.icon != 0) {
        framework::IconResource icon(line.icon, icon_size);
        icon.draw(dc, text.left, icon_top);
        text.left += icon_size + dpi.scale(IconGapDips);
    }

    if (auto font = reinterpret_cast<HFONT>(SendMessageW(control, WM_GETFONT, 0, 0))) {
        SelectObject(dc, font);
    }
    SetBkMode(dc, TRANSPARENT);
    if (!line.source.empty()) {
        // The name gives way, so the source stays readable
        int room = std::max(0, static_cast<int>(text.right - text.left) - dpi.scale(SourceGapDips) - text_width(dc, line.source));
        text.right = text.left + std::min(text_width(dc, line.text), room);
        SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
        draw_text(dc, line.source, {text.right + dpi.scale(SourceGapDips), text.top, area.right, text.bottom});
    }
    SetTextColor(dc, GetSysColor(line.gray ? COLOR_GRAYTEXT : COLOR_WINDOWTEXT));
    draw_text(dc, line.text, text);
}

}

void paint_link_line(const DRAWITEMSTRUCT& item, const LinkLine& line, double spinner_turn) {
    // The spinner repaints the line many times a second, so it's drawn off screen first, or its text would blink
    const RECT& area = item.rcItem;
    int width = std::max(1, static_cast<int>(area.right - area.left));
    int height = std::max(1, static_cast<int>(area.bottom - area.top));
    platform::win32::UniqueMemoryDc buffer(platform::win32::check(CreateCompatibleDC(item.hDC), "CreateCompatibleDC"));
    platform::win32::UniqueBitmap bitmap(platform::win32::check(CreateCompatibleBitmap(item.hDC, width, height), "CreateCompatibleBitmap"));
    {
        framework::SavedDc saved(buffer.get());
        SelectObject(buffer.get(), bitmap.get());
        draw_line(buffer.get(), item.hwndItem, {0, 0, width, height}, line, spinner_turn);
        platform::win32::check(BitBlt(item.hDC, area.left, area.top, width, height, buffer.get(), 0, 0, SRCCOPY), "BitBlt");
    }
}

}
