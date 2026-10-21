#include "ui/framework/BufferedPaint.h"

#include <algorithm>

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

BufferedPaint::BufferedPaint(HWND hwnd) :
        paint_(hwnd),
        buffer_(platform::win32::check(CreateCompatibleDC(paint_.dc()), "CreateCompatibleDC")),
        bitmap_(platform::win32::check(CreateCompatibleBitmap(paint_.dc(), width(), height()), "CreateCompatibleBitmap")),
        previous_bitmap_(SelectObject(buffer_.get(), bitmap_.get())) {
    // The buffer covers only the area to paint, at the same client coordinates
    SetViewportOrgEx(buffer_.get(), -paint_.area().left, -paint_.area().top, nullptr);
}

BufferedPaint::~BufferedPaint() {
    const RECT& area = paint_.area();
    BitBlt(paint_.dc(), area.left, area.top, width(), height(), buffer_.get(), area.left, area.top, SRCCOPY);
    SelectObject(buffer_.get(), previous_bitmap_);
}

HDC BufferedPaint::dc() const noexcept {
    return buffer_.get();
}

int BufferedPaint::width() const noexcept {
    return std::max(1, static_cast<int>(paint_.area().right - paint_.area().left));
}

int BufferedPaint::height() const noexcept {
    return std::max(1, static_cast<int>(paint_.area().bottom - paint_.area().top));
}

}
