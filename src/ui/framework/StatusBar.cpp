#include "ui/framework/StatusBar.h"

#include <algorithm>
#include <utility>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/Win32Error.h"
#include "ui/framework/SavedDc.h"

namespace westie::ui::framework {

StatusBar::StatusBar(HWND parent) :
        Control({
            .class_name = STATUSCLASSNAMEW,
            .style = WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | CCS_NORESIZE | CCS_NOPARENTALIGN,
            .parent = parent,
        }) {}

void StatusBar::set_parts(std::span<const int> left_widths, int right_width) {
    left_widths_.assign(left_widths.begin(), left_widths.end());
    right_width_ = right_width;
    apply_parts();
}

void StatusBar::set_text_padding(int padding) {
    text_padding_ = padding;
    platform::win32::check(InvalidateRect(hwnd(), nullptr, TRUE), "InvalidateRect");
}

void StatusBar::set_text(int part, std::wstring text, TextAlignment alignment) {
    if (std::cmp_greater_equal(part, texts_.size())) {
        texts_.resize(part + 1);
    }
    texts_[part] = {.text = std::move(text), .alignment = alignment};

    // The native part text sits 3 px from the edge with no way to pad it, so the parent's WM_DRAWITEM draws it. The
    // right part would draw its own separator at the window's edge.
    int flags = SBT_OWNERDRAW | (is_right_part(part) ? SBT_NOBORDERS : 0);
    platform::win32::check(SendMessageW(hwnd(), SB_SETTEXTW, part | flags, 0), "SB_SETTEXTW");
    RECT bounds = part_rect(part).to_rect();
    platform::win32::check(InvalidateRect(hwnd(), &bounds, TRUE), "InvalidateRect");
}

void StatusBar::draw(const DRAWITEMSTRUCT& item) const {
    if (item.itemID >= texts_.size()) {
        return;
    }
    const PartText& part = texts_[item.itemID];

    SavedDc saved(item.hDC);
    if (auto font = reinterpret_cast<HFONT>(SendMessageW(hwnd(), WM_GETFONT, 0, 0))) {
        SelectObject(item.hDC, font);
    }
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, GetSysColor(COLOR_BTNTEXT));

    // The rect for a part that runs to the edge reaches under the size grip; the bar's own rect for it stops there
    RECT bounds = item.rcItem;
    bounds.left += text_padding_;
    bounds.right = std::min(bounds.right, static_cast<LONG>(part_rect(static_cast<int>(item.itemID)).right)) - text_padding_;
    UINT format = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS | (part.alignment == TextAlignment::Center ? DT_CENTER : DT_LEFT);
    DrawTextW(item.hDC, part.text.c_str(), static_cast<int>(part.text.size()), &bounds, format);
}

LRESULT StatusBar::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = Control::handle_message(message, wparam, lparam);
    if (message == WM_SIZE) {
        apply_parts();
    }
    return result;
}

void StatusBar::apply_parts() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");

    // The left parts, then one that takes up the slack, then the right part. -1 runs that one to the edge without a
    // separator, and its rect, which the text is drawn in, stops at the size grip.
    std::vector<int> right_edges;
    int right = 0;
    for (int width : left_widths_) {
        right += width;
        right_edges.push_back(right);
    }
    right_edges.push_back(std::max(right, static_cast<int>(client.right) - right_width_));
    right_edges.push_back(-1);
    platform::win32::check(SendMessageW(hwnd(), SB_SETPARTS, right_edges.size(), reinterpret_cast<LPARAM>(right_edges.data())), "SB_SETPARTS");
}

bool StatusBar::is_right_part(int part) const noexcept {
    return std::cmp_equal(part, left_widths_.size() + 1);
}

Rect StatusBar::part_rect(int part) const {
    RECT rect;
    platform::win32::check(SendMessageW(hwnd(), SB_GETRECT, part, reinterpret_cast<LPARAM>(&rect)), "SB_GETRECT");
    return Rect::from_rect(rect);
}

}
