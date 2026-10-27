#include "ui/framework/WordBreaks.h"

#include <windows.h>

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

namespace {

enum class CharClass { Space, Word, Other };

CharClass char_class(wchar_t c) {
    WORD type = 0;
    platform::win32::check(GetStringTypeW(CT_CTYPE1, &c, 1, &type), "GetStringTypeW");
    if (type & C1_SPACE) {
        return CharClass::Space;
    }
    return type & (C1_ALPHA | C1_DIGIT) ? CharClass::Word : CharClass::Other;
}

}

std::size_t word_start_before(std::wstring_view text, std::size_t caret) {
    std::size_t start = caret;
    while (start > 0 && char_class(text[start - 1]) == CharClass::Space) {
        --start;
    }
    if (start > 0) {
        CharClass word = char_class(text[start - 1]);
        while (start > 0 && char_class(text[start - 1]) == word) {
            --start;
        }
    }
    return start;
}

std::size_t word_end_after(std::wstring_view text, std::size_t caret) {
    std::size_t end = caret;
    if (end < text.size() && char_class(text[end]) != CharClass::Space) {
        CharClass word = char_class(text[end]);
        while (end < text.size() && char_class(text[end]) == word) {
            ++end;
        }
    }
    while (end < text.size() && char_class(text[end]) == CharClass::Space) {
        ++end;
    }
    return end;
}

}
