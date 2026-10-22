#include "platform/win32/Clipboard.h"

#include <cstring>

#include "platform/win32/ClipboardScope.h"
#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

UniqueGlobal copy_to_global(std::wstring_view text) {
    std::size_t size = (text.size() + 1) * sizeof(wchar_t);
    UniqueGlobal memory(check(GlobalAlloc(GMEM_MOVEABLE, size), "GlobalAlloc"));
    auto* buffer = static_cast<wchar_t*>(check(GlobalLock(memory.get()), "GlobalLock"));
    std::memcpy(buffer, text.data(), text.size() * sizeof(wchar_t));
    buffer[text.size()] = L'\0';
    GlobalUnlock(memory.get());
    return memory;
}

}

bool set_clipboard_text(HWND owner, std::wstring_view text) {
    UniqueGlobal memory = copy_to_global(text);
    // Another program may hold the clipboard for a moment
    ClipboardScope clipboard(owner);
    if (!clipboard.open()) {
        return false;
    }
    check(EmptyClipboard(), "EmptyClipboard");
    check(SetClipboardData(CF_UNICODETEXT, memory.get()), "SetClipboardData");
    // The clipboard owns the memory now
    memory.release();
    return true;
}

}
