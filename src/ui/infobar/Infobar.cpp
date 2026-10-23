#include "ui/infobar/Infobar.h"

#include <algorithm>
#include <utility>

#include <windowsx.h>

#include "ResourceIds.h"
#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Size.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui {

namespace {

constexpr int HeightDips = 32;
constexpr int LeftPaddingDips = 10;
constexpr int RightPaddingDips = 6;
constexpr int IconDips = 16;
constexpr int GapDips = 8;
constexpr int CloseBoxDips = 24;
constexpr int CloseIconDips = 12;
constexpr int ButtonPaddingDips = 4;
// A hairline stays one pixel at every scale
constexpr int HairlinePixels = 1;
constexpr COLORREF InfobarColor = RGB(0xFF, 0xF4, 0xCE);
constexpr COLORREF RuleColor = RGB(0xEA, 0xDB, 0x9F);
constexpr WORD DetailsCommand = 1;

framework::Rect centered(const framework::Rect& slot, int width, int height) {
    int left = slot.left + (slot.width() - width) / 2;
    int top = slot.top + (slot.height() - height) / 2;
    return {left, top, left + width, top + height};
}

}

Infobar::Infobar(HWND parent, std::function<void()> on_details, std::function<void()> on_dismiss) :
        Window(window_class(), {.title = L"Infobar", .style = WS_CHILD | WS_CLIPCHILDREN, .parent = parent}),
        on_details_(std::move(on_details)),
        on_dismiss_(std::move(on_dismiss)),
        message_(hwnd(), L"", {.background = InfobarColor}),
        details_(hwnd(), {.text = L"Details", .command = DetailsCommand, .compact = true}) {}

int Infobar::height() const noexcept {
    return height_;
}

void Infobar::apply_dpi(framework::Dpi dpi, const framework::MessageFont& font) {
    message_.set_font(font.handle());
    line_height_ = font.line_height();
    details_.apply_dpi(dpi, font);
    warn_.emplace(IDI_WARN, dpi.scale(IconDips));
    close_.emplace(IDI_CLOSE, dpi.scale(CloseIconDips));
    // Taller when the system font needs a taller button
    height_ = std::max(dpi.scale(HeightDips), details_.size().height + 2 * dpi.scale(ButtonPaddingDips)) + HairlinePixels;
    layout();
}

void Infobar::set_message(std::wstring message) {
    message_.set_text(std::move(message));
}

LRESULT Infobar::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:           layout(); return 0;
    case WM_PAINT:          paint(); return 0;
    case WM_LBUTTONDOWN:    on_left_button_down(POINT{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)}); return 0;
    case WM_LBUTTONUP:      on_left_button_up(POINT{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)}); return 0;
    case WM_CAPTURECHANGED: close_pressed_ = false; return 0;
    case WM_COMMAND:        on_command(wparam); return 0;
    default:                return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& Infobar::window_class() {
    // The buttons' themed corners show the class background, which has to be the strip's yellow
    static platform::win32::UniqueBrush background(platform::win32::check(CreateSolidBrush(InfobarColor), "CreateSolidBrush"));
    static framework::WindowClass window_class({.name = L"WestieLib.Infobar", .background = background.get()});
    return window_class;
}

framework::Rect Infobar::strip() const {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::Rect area = framework::Rect::from_rect(client);
    area.take_bottom(HairlinePixels);
    return area;
}

framework::Rect Infobar::close_box() const {
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    framework::Rect area = strip();
    area.take_right(dpi.scale(RightPaddingDips));
    return centered(area.take_right(dpi.scale(CloseBoxDips)), dpi.scale(CloseBoxDips), dpi.scale(CloseBoxDips));
}

void Infobar::layout() {
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    framework::Rect area = strip();
    area.take_left(dpi.scale(LeftPaddingDips) + dpi.scale(IconDips) + dpi.scale(GapDips));
    area.take_right(dpi.scale(RightPaddingDips) + dpi.scale(CloseBoxDips) + dpi.scale(GapDips));
    framework::Size button = details_.size();
    framework::Rect details = centered(area.take_right(button.width), button.width, button.height);
    area.take_right(dpi.scale(GapDips));

    framework::DeferredPositions positions;
    positions.place(message_.hwnd(), centered(area, area.width(), line_height_));
    positions.place(details_.hwnd(), details);
    positions.apply();
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

void Infobar::paint() const {
    framework::BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    SetDCBrushColor(paint.dc(), RuleColor);
    FillRect(paint.dc(), &client, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    framework::Rect area = strip();
    RECT fill = area.to_rect();
    SetDCBrushColor(paint.dc(), InfobarColor);
    FillRect(paint.dc(), &fill, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    if (!warn_ || !close_) {
        return;
    }

    framework::Dpi dpi = framework::Dpi::of(hwnd());
    warn_->draw(paint.dc(), area.left + dpi.scale(LeftPaddingDips), area.top + (area.height() - warn_->size()) / 2);
    framework::Rect box = close_box();
    close_->draw(paint.dc(), box.left + (box.width() - close_->size()) / 2, box.top + (box.height() - close_->size()) / 2);
}

void Infobar::on_left_button_down(POINT point) {
    RECT box = close_box().to_rect();
    if (PtInRect(&box, point)) {
        close_pressed_ = true;
        SetCapture(hwnd());
    }
}

void Infobar::on_left_button_up(POINT point) {
    if (!close_pressed_) {
        return;
    }
    // Cleared first, so the WM_CAPTURECHANGED that ReleaseCapture sends finds nothing to cancel
    close_pressed_ = false;
    platform::win32::check(ReleaseCapture(), "ReleaseCapture");
    RECT box = close_box().to_rect();
    if (PtInRect(&box, point)) {
        on_dismiss_();
    }
}

void Infobar::on_command(WPARAM wparam) const {
    if (LOWORD(wparam) == DetailsCommand && HIWORD(wparam) == BN_CLICKED) {
        on_details_();
    }
}

}
