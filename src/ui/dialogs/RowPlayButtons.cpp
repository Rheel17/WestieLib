#include "ui/dialogs/RowPlayButtons.h"

#include <algorithm>
#include <utility>

#include <windowsx.h>
#include <commctrl.h>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/Rect.h"
#include "ui/framework/SavedDc.h"

namespace westie::ui {

namespace {

constexpr int BoxDips = 18;
constexpr int IconDips = 12;
constexpr UINT_PTR TipTool = 1;
constexpr wchar_t Tip[] = L"Play on YouTube Music";
constexpr COLORREF HotColor = RGB(0xE5, 0xF3, 0xFF);
constexpr COLORREF PressedColor = RGB(0xCC, 0xE4, 0xF7);
constexpr COLORREF FrameColor = RGB(0xCC, 0xE8, 0xFF);
constexpr COLORREF SelectedFrameColor = RGB(0x99, 0xD1, 0xFF);

}

RowPlayButtons::RowPlayButtons(HWND list, int column, std::function<void(std::size_t)> on_play) :
        list_(list), column_(column), on_play_(std::move(on_play)), tooltip_(list) {
    apply_dpi();
}

void RowPlayButtons::apply_dpi() {
    icon_.emplace(IDI_PLAY, framework::Dpi::of(list_).scale(IconDips));
    platform::win32::check(InvalidateRect(list_, nullptr, FALSE), "InvalidateRect");
}

std::optional<LRESULT> RowPlayButtons::on_mouse(UINT message, LPARAM lparam) {
    std::optional<std::size_t> hot_before = mouse_.hot();
    switch (message) {
    case WM_MOUSEMOVE:      repaint(mouse_.move(list_, item_at(lparam)), hot_before); return std::nullopt;
    case WM_MOUSELEAVE:     repaint(mouse_.leave(), hot_before); return std::nullopt;
    case WM_CAPTURECHANGED: repaint(mouse_.cancel(), hot_before); return std::nullopt;
    // A double click on a button is a second click, which the list would take for choosing the row
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK: {
        std::optional<std::size_t> item = item_at(lparam);
        if (!item) {
            return std::nullopt;
        }
        repaint(mouse_.press(list_, item), hot_before);
        return 0;
    }
    case WM_LBUTTONUP: {
        if (!mouse_.held()) {
            return std::nullopt;
        }
        std::optional<std::size_t> clicked = mouse_.release(list_, item_at(lparam));
        repaint(true, hot_before);
        if (clicked) {
            on_play_(*clicked);
        }
        return 0;
    }
    default: return std::nullopt;
    }
}

void RowPlayButtons::follow_cursor() {
    POINT cursor;
    platform::win32::check(GetCursorPos(&cursor), "GetCursorPos");
    platform::win32::check(ScreenToClient(list_, &cursor), "ScreenToClient");
    std::optional<std::size_t> hot_before = mouse_.hot();
    repaint(mouse_.move(list_, item_at(cursor)), hot_before);
}

void RowPlayButtons::paint(HDC dc, int item) const {
    if (!icon_ || item >= ListView_GetItemCount(list_)) {
        return;
    }
    framework::SavedDc saved(dc);
    RECT area = box(item);
    if (mouse_.hot() == static_cast<std::size_t>(item)) {
        bool selected = ListView_GetItemState(list_, item, LVIS_SELECTED) != 0;
        SetDCBrushColor(dc, mouse_.pressed() ? PressedColor : HotColor);
        FillRect(dc, &area, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        SetDCBrushColor(dc, selected ? SelectedFrameColor : FrameColor);
        FrameRect(dc, &area, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    }
    int size = icon_->size();
    icon_->draw(dc, area.left + (area.right - area.left - size) / 2, area.top + (area.bottom - area.top - size) / 2);
}

RECT RowPlayButtons::cell(int item) const {
    // The message reads which rect from left, and which column from top
    RECT cell{LVIR_BOUNDS, column_, 0, 0};
    platform::win32::check(SendMessageW(list_, LVM_GETSUBITEMRECT, item, reinterpret_cast<LPARAM>(&cell)), "LVM_GETSUBITEMRECT");
    return cell;
}

RECT RowPlayButtons::box(int item) const {
    RECT cell = this->cell(item);
    int side = std::min({framework::Dpi::of(list_).scale(BoxDips), static_cast<int>(cell.right - cell.left), static_cast<int>(cell.bottom - cell.top)});
    int left = cell.left + (cell.right - cell.left - side) / 2;
    int top = cell.top + (cell.bottom - cell.top - side) / 2;
    return {left, top, left + side, top + side};
}

std::optional<std::size_t> RowPlayButtons::item_at(POINT point) const {
    LVHITTESTINFO hit{};
    hit.pt = point;
    SendMessageW(list_, LVM_SUBITEMHITTEST, 0, reinterpret_cast<LPARAM>(&hit));
    if (hit.iItem < 0 || hit.iSubItem != column_) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(hit.iItem);
}

std::optional<std::size_t> RowPlayButtons::item_at(LPARAM point) const {
    return item_at(POINT{GET_X_LPARAM(point), GET_Y_LPARAM(point)});
}

void RowPlayButtons::repaint(bool changed, std::optional<std::size_t> hot_before) {
    if (!changed) {
        return;
    }
    int count = ListView_GetItemCount(list_);
    for (std::optional<std::size_t> item : {hot_before, mouse_.hot()}) {
        if (item && std::cmp_less(*item, count)) {
            RECT area = box(static_cast<int>(*item));
            platform::win32::check(InvalidateRect(list_, &area, FALSE), "InvalidateRect");
        }
    }
    if (mouse_.hot() == hot_before) {
        return;
    }
    if (std::optional<std::size_t> hot = mouse_.hot(); hot && std::cmp_less(*hot, count)) {
        tooltip_.set_tool(TipTool, framework::Rect::from_rect(cell(static_cast<int>(*hot))), Tip);
    } else {
        tooltip_.remove_tool(TipTool);
    }
}

}
