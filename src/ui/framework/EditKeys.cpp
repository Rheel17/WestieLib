#include "ui/framework/EditKeys.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/Win32Error.h"
#include "ui/framework/EditHistory.h"
#include "ui/framework/PendingException.h"
#include "ui/framework/WordBreaks.h"

namespace westie::ui::framework {

namespace {

constexpr UINT_PTR SubclassId = 1;
constexpr WPARAM BackspaceChar = 0x08;
constexpr WPARAM CtrlYChar = 0x19;
constexpr WPARAM CtrlZChar = 0x1A;
constexpr WPARAM FirstTypedChar = 0x20;
// What TranslateMessage makes of Ctrl+Backspace, which the edit would type as a box
constexpr WPARAM CtrlBackspaceChar = 0x7F;

enum class WordSide { Before, After };

bool is_down(int key) {
    return GetKeyState(key) < 0;
}

bool is_read_only(HWND edit) {
    return (GetWindowLongPtrW(edit, GWL_STYLE) & ES_READONLY) != 0;
}

std::wstring text_of(HWND edit) {
    std::wstring text;
    text.resize_and_overwrite(GetWindowTextLengthW(edit), [edit](wchar_t* buffer, std::size_t size) {
        return static_cast<std::size_t>(GetWindowTextW(edit, buffer, static_cast<int>(size) + 1));
    });
    return text;
}

EditSelection selection_of(HWND edit) {
    DWORD start = 0;
    DWORD end = 0;
    SendMessageW(edit, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
    return {.start = start, .end = end};
}

void delete_word(HWND edit, WordSide side) {
    if (is_read_only(edit)) {
        return;
    }
    EditSelection selection = selection_of(edit);
    if (selection.start == selection.end) {
        std::wstring text = text_of(edit);
        if (side == WordSide::Before) {
            selection.start = word_start_before(text, selection.end);
        } else {
            selection.end = word_end_after(text, selection.start);
        }
        if (selection.start == selection.end) {
            return;
        }
        SendMessageW(edit, EM_SETSEL, selection.start, selection.end);
    }
    // Replacing the selection notifies EN_CHANGE like typing
    SendMessageW(edit, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(L""));
}

void restore(HWND edit, const std::optional<EditState>& state) {
    if (!state) {
        return;
    }
    // Past this subclass, which would take it for the app setting the text and start a new history
    DefSubclassProc(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(state->text.c_str()));
    SendMessageW(edit, EM_SETSEL, state->selection.start, state->selection.end);
    SendMessageW(edit, EM_SCROLLCARET, 0, 0);
}

bool handle_key(HWND edit, EditHistory& history, UINT message, WPARAM wparam) {
    if (message == WM_CHAR && wparam == CtrlBackspaceChar) {
        delete_word(edit, WordSide::Before);
        return true;
    }
    if (message == WM_KEYDOWN && wparam == VK_DELETE && is_down(VK_CONTROL) && !is_down(VK_SHIFT) && !is_down(VK_MENU)) {
        delete_word(edit, WordSide::After);
        return true;
    }
    if (message == WM_CHAR && (wparam == CtrlZChar || wparam == CtrlYChar)) {
        if (!is_read_only(edit)) {
            restore(edit, wparam == CtrlZChar ? history.undo() : history.redo());
        }
        return true;
    }
    // The edit's own undo keeps one step, which a second undo redoes
    switch (message) {
    case WM_UNDO:
    case EM_UNDO:
    case EM_CANUNDO:
    case WM_CONTEXTMENU: return true;
    case WM_SYSKEYDOWN:  return wparam == VK_BACK;
    default:             return false;
    }
}

bool may_change_text(UINT message) {
    switch (message) {
    case WM_CHAR:
    case WM_KEYDOWN:
    case WM_PASTE:
    case WM_CUT:
    case WM_CLEAR:
    case EM_REPLACESEL:
    case WM_IME_COMPOSITION: return true;
    default:                 return false;
    }
}

EditKind change_kind(UINT message, WPARAM wparam, EditSelection selection) {
    if (selection.start != selection.end) {
        return EditKind::Other;
    }
    if ((message == WM_CHAR && wparam == BackspaceChar) || (message == WM_KEYDOWN && wparam == VK_DELETE)) {
        return EditKind::Deleting;
    }
    if (message == WM_CHAR && wparam >= FirstTypedChar) {
        return EditKind::Typing;
    }
    return EditKind::Other;
}

LRESULT handle_message(HWND edit, EditHistory& history, UINT message, WPARAM wparam, LPARAM lparam) {
    if (handle_key(edit, history, message, wparam)) {
        return 0;
    }
    if (message == WM_SETTEXT) {
        LRESULT result = DefSubclassProc(edit, message, wparam, lparam);
        // Setting the text it already has, as committing the library folder often does, keeps the history
        if (std::wstring text = text_of(edit); text != history.text()) {
            history.reset(std::move(text));
        }
        return result;
    }
    if (!may_change_text(message)) {
        return DefSubclassProc(edit, message, wparam, lparam);
    }

    EditSelection before = selection_of(edit);
    EditKind kind = change_kind(message, wparam, before);
    LRESULT result = DefSubclassProc(edit, message, wparam, lparam);
    // A change made inside this one, as Esc emptying the search, is already recorded
    if (std::wstring text = text_of(edit); text != history.text()) {
        history.record(before, {.text = std::move(text), .selection = selection_of(edit)}, kind);
    }
    return result;
}

LRESULT CALLBACK subclass_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data) {
    auto* history = reinterpret_cast<EditHistory*>(data);
    if (message == WM_NCDESTROY) {
        std::unique_ptr<EditHistory> owned(history);
        RemoveWindowSubclass(hwnd, subclass_proc, id);
        return DefSubclassProc(hwnd, message, wparam, lparam);
    }

    try {
        return handle_message(hwnd, *history, message, wparam, lparam);
    } catch (...) {
        PendingException::capture();
        return 0;
    }
}

}

void add_edit_keys(HWND edit) {
    auto history = std::make_unique<EditHistory>(text_of(edit));
    platform::win32::check(SetWindowSubclass(edit, subclass_proc, SubclassId, reinterpret_cast<DWORD_PTR>(history.get())), "SetWindowSubclass");
    // The subclass owns it from here, until WM_NCDESTROY
    static_cast<void>(history.release());
}

}
