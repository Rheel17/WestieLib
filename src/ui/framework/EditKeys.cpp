#include "ui/framework/EditKeys.h"

#include <cstddef>
#include <string>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/Win32Error.h"
#include "ui/framework/PendingException.h"
#include "ui/framework/WordBreaks.h"

namespace westie::ui::framework {

namespace {

constexpr UINT_PTR SubclassId = 1;
// What TranslateMessage makes of Ctrl+Backspace, which the edit would type as a box
constexpr WPARAM CtrlBackspaceChar = 0x7F;
constexpr WPARAM CtrlZChar = 0x1A;

enum class WordSide { Before, After };

bool is_down(int key) {
    return GetKeyState(key) < 0;
}

std::wstring text_of(HWND edit) {
    std::wstring text;
    text.resize_and_overwrite(GetWindowTextLengthW(edit), [edit](wchar_t* buffer, std::size_t size) {
        return static_cast<std::size_t>(GetWindowTextW(edit, buffer, static_cast<int>(size) + 1));
    });
    return text;
}

void delete_word(HWND edit, WordSide side) {
    if (GetWindowLongPtrW(edit, GWL_STYLE) & ES_READONLY) {
        return;
    }
    DWORD start = 0;
    DWORD end = 0;
    SendMessageW(edit, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
    if (start == end) {
        std::wstring text = text_of(edit);
        if (side == WordSide::Before) {
            start = static_cast<DWORD>(word_start_before(text, end));
        } else {
            end = static_cast<DWORD>(word_end_after(text, start));
        }
        if (start == end) {
            return;
        }
        SendMessageW(edit, EM_SETSEL, start, end);
    }
    // Replacing the selection notifies EN_CHANGE like typing
    SendMessageW(edit, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(L""));
}

LRESULT CALLBACK subclass_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR) {
    if (message == WM_NCDESTROY) {
        RemoveWindowSubclass(hwnd, subclass_proc, id);
        return DefSubclassProc(hwnd, message, wparam, lparam);
    }

    try {
        if (handle_edit_key(hwnd, message, wparam)) {
            return 0;
        }
    } catch (...) {
        PendingException::capture();
        return 0;
    }
    return DefSubclassProc(hwnd, message, wparam, lparam);
}

}

bool handle_edit_key(HWND edit, UINT message, WPARAM wparam) {
    if (message == WM_CHAR && wparam == CtrlBackspaceChar) {
        delete_word(edit, WordSide::Before);
        return true;
    }
    if (message == WM_KEYDOWN && wparam == VK_DELETE && is_down(VK_CONTROL) && !is_down(VK_SHIFT) && !is_down(VK_MENU)) {
        delete_word(edit, WordSide::After);
        return true;
    }
    // The edit's own undo keeps one step, which a second Ctrl+Z redoes, so undo is off until the app keeps a history
    switch (message) {
    case WM_UNDO:
    case EM_UNDO:
    case EM_CANUNDO:    return true;
    case WM_CHAR:       return wparam == CtrlZChar;
    case WM_SYSKEYDOWN: return wparam == VK_BACK;
    }
    return false;
}

void add_edit_keys(HWND edit) {
    platform::win32::check(SetWindowSubclass(edit, subclass_proc, SubclassId, 0), "SetWindowSubclass");
}

}
