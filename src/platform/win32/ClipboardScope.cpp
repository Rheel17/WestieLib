#include "platform/win32/ClipboardScope.h"

namespace westie::platform::win32 {

ClipboardScope::ClipboardScope(HWND owner) noexcept : open_(OpenClipboard(owner) != FALSE) {}

ClipboardScope::~ClipboardScope() {
    if (open_) {
        CloseClipboard();
    }
}

bool ClipboardScope::open() const noexcept {
    return open_;
}

}
