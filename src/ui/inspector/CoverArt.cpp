#include "ui/inspector/CoverArt.h"

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui {

namespace {

constexpr int NoteDips = 28;
// A hairline stays one pixel at every scale
constexpr int BorderPixels = 1;
constexpr COLORREF FaceColor = RGB(0xF0, 0xF0, 0xF0);
constexpr COLORREF BorderColor = RGB(0xE5, 0xE5, 0xE5);

}

CoverArt::CoverArt(HWND parent) : Window(window_class(), {.style = WS_CHILD | WS_VISIBLE, .parent = parent}) {}

void CoverArt::apply_dpi(framework::Dpi dpi) {
    note_.emplace(IDI_NOTE, dpi.scale(NoteDips));
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

LRESULT CoverArt::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_PAINT: paint(); return 0;
    default:       return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& CoverArt::window_class() {
    // The paint fills the background, so the placeholder isn't erased first and doesn't blink
    static framework::WindowClass window_class({.name = L"WestieLib.CoverArt", .style = CS_HREDRAW | CS_VREDRAW});
    return window_class;
}

void CoverArt::paint() const {
    framework::BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    SetDCBrushColor(paint.dc(), BorderColor);
    FillRect(paint.dc(), &client, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    RECT face{client.left + BorderPixels, client.top + BorderPixels, client.right - BorderPixels, client.bottom - BorderPixels};
    SetDCBrushColor(paint.dc(), FaceColor);
    FillRect(paint.dc(), &face, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    if (note_) {
        note_->draw(paint.dc(), (client.right - note_->size()) / 2, (client.bottom - note_->size()) / 2);
    }
}

}
