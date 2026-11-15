#include "ui/framework/DialogSizing.h"

#include <algorithm>
#include <utility>

#include "platform/win32/Win32Error.h"
#include "ui/framework/DeferredPositions.h"

namespace westie::ui::framework {

namespace {

Size client_size(HWND window) {
    RECT client;
    platform::win32::check(GetClientRect(window, &client), "GetClientRect");
    return {client.right, client.bottom};
}

Rect window_rect(HWND window) {
    RECT bounds;
    platform::win32::check(GetWindowRect(window, &bounds), "GetWindowRect");
    return Rect::from_rect(bounds);
}

int rescale(int pixels, Dpi from, Dpi to) {
    return MulDiv(pixels, static_cast<int>(to.value()), static_cast<int>(from.value()));
}

std::pair<int, int> anchored(int start, int end, Anchor anchor, int growth) {
    switch (anchor) {
    case Anchor::Start:   return {start, end};
    case Anchor::Stretch: return {start, end + growth};
    case Anchor::End:     return {start + growth, end + growth};
    }
    return {start, end};
}

}

DialogSizing::DialogSizing(HWND dialog, std::span<const ControlAnchor> anchors) :
        dialog_(dialog), dpi_(Dpi::of(dialog)), client_(client_size(dialog)) {
    Rect bounds = window_rect(dialog);
    minimum_dips_ = {dpi_.unscale(bounds.width()), dpi_.unscale(bounds.height())};
    for (const ControlAnchor& anchor : anchors) {
        HWND control = platform::win32::check(GetDlgItem(dialog, anchor.id), "GetDlgItem");
        RECT rect = window_rect(control).to_rect();
        // Mapping can rightly return 0, when the client area starts at the screen's origin
        MapWindowPoints(nullptr, dialog, reinterpret_cast<POINT*>(&rect), 2);
        controls_.push_back({.control = control, .rect = Rect::from_rect(rect), .horizontal = anchor.horizontal, .vertical = anchor.vertical});
    }
}

void DialogSizing::resize(Size window_dips) const {
    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    platform::win32::check(GetMonitorInfoW(MonitorFromWindow(dialog_, MONITOR_DEFAULTTONEAREST), &monitor), "GetMonitorInfoW");
    Rect work = Rect::from_rect(monitor.rcWork);
    Dpi dpi = Dpi::of(dialog_);
    Size minimum = dpi.scale(minimum_dips_);
    Size wanted = dpi.scale(window_dips);
    int width = std::clamp(wanted.width, minimum.width, std::max(minimum.width, work.width()));
    int height = std::clamp(wanted.height, minimum.height, std::max(minimum.height, work.height()));
    platform::win32::check(SetWindowPos(dialog_, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE), "SetWindowPos");
}

Size DialogSizing::window_dips() const {
    Dpi dpi = Dpi::of(dialog_);
    Rect bounds = window_rect(dialog_);
    return {dpi.unscale(bounds.width()), dpi.unscale(bounds.height())};
}

void DialogSizing::limit(MINMAXINFO& info) const {
    Size minimum = Dpi::of(dialog_).scale(minimum_dips_);
    info.ptMinTrackSize = {minimum.width, minimum.height};
}

void DialogSizing::place() const {
    // On another monitor the controls keep the template's places in proportion
    Dpi dpi = Dpi::of(dialog_);
    Size client = client_size(dialog_);
    int grow_x = client.width - rescale(client_.width, dpi_, dpi);
    int grow_y = client.height - rescale(client_.height, dpi_, dpi);
    DeferredPositions positions;
    for (const Placed& placed : controls_) {
        auto [left, right] = anchored(rescale(placed.rect.left, dpi_, dpi), rescale(placed.rect.right, dpi_, dpi), placed.horizontal, grow_x);
        auto [top, bottom] = anchored(rescale(placed.rect.top, dpi_, dpi), rescale(placed.rect.bottom, dpi_, dpi), placed.vertical, grow_y);
        positions.place(placed.control, Rect{left, top, right, bottom});
    }
    positions.apply();
}

}
