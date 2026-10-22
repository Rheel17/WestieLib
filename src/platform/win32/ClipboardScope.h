#pragma once

#include <windows.h>

namespace westie::platform::win32 {

class ClipboardScope {
public:
    explicit ClipboardScope(HWND owner) noexcept;
    ~ClipboardScope();

    ClipboardScope(const ClipboardScope&) = delete;
    ClipboardScope& operator=(const ClipboardScope&) = delete;

    bool open() const noexcept;

private:
    bool open_ = false;
};

}
