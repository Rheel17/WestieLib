#pragma once

#include <windows.h>

#include "platform/win32/UniqueHandle.h"
#include "ui/framework/PaintScope.h"

namespace westie::ui::framework {

class BufferedPaint {
public:
    explicit BufferedPaint(HWND hwnd);
    ~BufferedPaint();

    BufferedPaint(const BufferedPaint&) = delete;
    BufferedPaint& operator=(const BufferedPaint&) = delete;

    HDC dc() const noexcept;

private:
    int width() const noexcept;
    int height() const noexcept;

    PaintScope paint_;
    platform::win32::UniqueMemoryDc buffer_;
    platform::win32::UniqueBitmap bitmap_;
    HGDIOBJ previous_bitmap_ = nullptr;
};

}
