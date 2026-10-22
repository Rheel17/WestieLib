#include "ui/framework/StatusBar.h"

#include <algorithm>
#include <array>
#include <numeric>
#include <utility>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/SavedDc.h"

namespace westie::ui::framework {

namespace {

constexpr int WholePercent = 100;

int measure(HDC dc, std::wstring_view text) {
    SIZE size{};
    platform::win32::check(GetTextExtentPoint32W(dc, text.data(), static_cast<int>(text.size()), &size), "GetTextExtentPoint32W");
    return size.cx;
}

std::vector<int> widths_in(std::span<const PartWidth> parts, int bar_width) {
    std::vector<int> widths;
    for (const PartWidth& part : parts) {
        widths.push_back(std::max(part.minimum, bar_width * part.percent / WholePercent));
    }
    return widths;
}

}

StatusBar::StatusBar(HWND parent) :
        Control({
            .class_name = STATUSCLASSNAMEW,
            .style = WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | CCS_NORESIZE | CCS_NOPARENTALIGN,
            .parent = parent,
        }),
        tooltip_(hwnd()) {}

void StatusBar::set_parts(StatusParts parts) {
    parts_ = std::move(parts);
    apply_parts();
}

void StatusBar::set_spacing(int padding, int gap) {
    padding_ = padding;
    gap_ = gap;
    place_embedded();
    platform::win32::check(InvalidateRect(hwnd(), nullptr, TRUE), "InvalidateRect");
}

void StatusBar::set_text(int part, std::wstring text, const PartFormat& format) {
    set_content(part, {.text = std::move(text)}, format);
}

void StatusBar::set_content(int part, PartContent content, const PartFormat& format) {
    if (std::cmp_greater_equal(part, texts_.size())) {
        texts_.resize(part + 1);
    }
    texts_[part] = {.content = std::move(content), .format = format};

    // The native part text sits 3 px from the edge with no way to pad it, so the parent's WM_DRAWITEM draws it. The
    // last part would draw its own separator at the window's edge.
    int flags = SBT_OWNERDRAW | (is_last_part(part) ? SBT_NOBORDERS : 0);
    platform::win32::check(SendMessageW(hwnd(), SB_SETTEXTW, part | flags, 0), "SB_SETTEXTW");
    RECT bounds = part_rect(part).to_rect();
    platform::win32::check(InvalidateRect(hwnd(), &bounds, TRUE), "InvalidateRect");
}

void StatusBar::set_tip(int part, std::wstring text) {
    std::wstring& tip = tips_[part];
    tip = std::move(text);
    tooltip_.set_tool(part, part_rect(part), tip);
}

void StatusBar::embed(int part, HWND window, std::optional<Size> size) {
    embedded_[part] = {.window = window, .size = size};
    place_embedded();
}

int StatusBar::part_width_for(const PartContent& content) const {
    platform::win32::UniqueMemoryDc dc(platform::win32::check(CreateCompatibleDC(nullptr), "CreateCompatibleDC"));
    if (auto font = reinterpret_cast<HFONT>(SendMessageW(hwnd(), WM_GETFONT, 0, 0))) {
        SelectObject(dc.get(), font);
    }
    // A part's rect starts this far after the edge before it: horizontal border, vertical border, gap between parts
    std::array<int, 3> borders{};
    platform::win32::check(SendMessageW(hwnd(), SB_GETBORDERS, 0, reinterpret_cast<LPARAM>(borders.data())), "SB_GETBORDERS");
    int width = borders[2] + 2 * padding_ + measure(dc.get(), content.text);
    if (content.icon) {
        width += gap_ + content.icon->size();
    }
    if (!content.trailing.empty()) {
        width += gap_ + measure(dc.get(), content.trailing);
    }
    return width;
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

    // The item rect for a part that runs to the edge reaches under the size grip; the bar's own rect for it stops there.
    // The bar's rect is also what part_width_for measures against.
    Rect padded = padded_rect(static_cast<int>(item.itemID));
    RECT bounds{padded.left, item.rcItem.top, padded.right, item.rcItem.bottom};
    const PartContent& content = part.content;
    UINT ellipsis = part.format.ellipsis == TextEllipsis::Path ? DT_PATH_ELLIPSIS : DT_END_ELLIPSIS;
    UINT alignment = part.format.alignment == TextAlignment::Center ? DT_CENTER : DT_LEFT;
    auto draw_text = [&](const std::wstring& text, RECT area) {
        DrawTextW(item.hDC, text.c_str(), static_cast<int>(text.size()), &area, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | ellipsis | alignment);
    };

    // With something after it, the text gets no more than it needs, and leaves the icon room when the part is narrow
    RECT text_bounds = bounds;
    int icon_width = content.icon ? gap_ + content.icon->size() : 0;
    if (content.icon || !content.trailing.empty()) {
        text_bounds.right = std::min<int>(bounds.left + measure(item.hDC, content.text), std::max<int>(bounds.left, bounds.right - icon_width));
    }
    draw_text(content.text, text_bounds);
    int right = text_bounds.right;
    if (content.icon) {
        content.icon->draw(item.hDC, right + gap_, (item.rcItem.top + item.rcItem.bottom - content.icon->size()) / 2);
        right += icon_width;
    }
    if (!content.trailing.empty()) {
        draw_text(content.trailing, {right + gap_, bounds.top, bounds.right, bounds.bottom});
    }
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

    // Each part takes its share of the bar, or its minimum. When the window is too narrow for that, the yielding part
    // gives up what's missing.
    int bar_width = client.right;
    std::vector<int> left_widths = widths_in(parts_.left, bar_width);
    std::vector<int> right_widths = widths_in(parts_.right, bar_width);
    int right_total = std::reduce(right_widths.begin(), right_widths.end());
    int room = std::max(0, bar_width - right_total);
    int missing = std::reduce(left_widths.begin(), left_widths.end()) - room;
    if (missing > 0 && std::cmp_less(parts_.yielding, left_widths.size())) {
        left_widths[parts_.yielding] = std::max(0, left_widths[parts_.yielding] - missing);
    }

    // The left parts, then one that takes up the slack, then the right parts. -1 runs the last one to the edge without
    // a separator, and its rect, which the text is drawn in, stops at the size grip.
    std::vector<int> right_edges;
    int right = 0;
    for (int width : left_widths) {
        right += width;
        right_edges.push_back(right);
    }
    right = std::max(right, bar_width - right_total);
    right_edges.push_back(right);
    for (std::size_t index = 0; index + 1 < right_widths.size(); ++index) {
        right += right_widths[index];
        right_edges.push_back(right);
    }
    right_edges.push_back(-1);
    platform::win32::check(SendMessageW(hwnd(), SB_SETPARTS, right_edges.size(), reinterpret_cast<LPARAM>(right_edges.data())), "SB_SETPARTS");

    place_embedded();
    place_tips();
}

void StatusBar::place_embedded() const {
    for (const auto& [part, embedded] : embedded_) {
        Rect bounds = padded_rect(part);
        if (!embedded.size || bounds.width() < embedded.size->width) {
            platform::win32::check(
                    SetWindowPos(embedded.window, nullptr, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE), "SetWindowPos"
            );
            continue;
        }
        Size size = *embedded.size;
        int top = bounds.top + (bounds.height() - size.height) / 2;
        platform::win32::check(
                SetWindowPos(embedded.window, nullptr, bounds.right - size.width, top, size.width, size.height, SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE),
                "SetWindowPos"
        );
    }
}

void StatusBar::place_tips() {
    for (const auto& [part, tip] : tips_) {
        tooltip_.set_tool(part, part_rect(part), tip);
    }
}

bool StatusBar::is_last_part(int part) const noexcept {
    return std::cmp_equal(part, parts_.left.size() + parts_.right.size());
}

Rect StatusBar::part_rect(int part) const {
    RECT rect;
    platform::win32::check(SendMessageW(hwnd(), SB_GETRECT, part, reinterpret_cast<LPARAM>(&rect)), "SB_GETRECT");
    return Rect::from_rect(rect);
}

Rect StatusBar::padded_rect(int part) const {
    Rect bounds = part_rect(part);
    return {bounds.left + padding_, bounds.top, bounds.right - padding_, bounds.bottom};
}

}
