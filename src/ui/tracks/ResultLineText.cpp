#include "ui/tracks/ResultLineText.h"

#include <format>

namespace westie::ui {

std::wstring result_line_text(int shown, int total, std::optional<std::wstring_view> playlist_name) {
    std::wstring text = std::format(L"Showing {} of {}", shown, total);
    if (playlist_name) {
        text += std::format(L" · in {}", *playlist_name);
    }
    return text;
}

}
