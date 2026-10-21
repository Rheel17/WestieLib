#include "ui/welcome/LibraryFolderText.h"

#include <algorithm>

namespace westie::ui {

namespace {

constexpr std::wstring_view Spaces = L" \t";
// Not allowed in a file or folder name; ':' may only follow the drive letter
constexpr std::wstring_view NameCharactersNotAllowed = L"<>:\"|?*";

std::wstring_view trim(std::wstring_view text) {
    std::size_t first = text.find_first_not_of(Spaces);
    if (first == std::wstring_view::npos) {
        return {};
    }
    return text.substr(first, text.find_last_not_of(Spaces) - first + 1);
}

// Explorer's "Copy as path" puts quotes around the path
std::wstring_view unquote(std::wstring_view text) {
    if (text.size() >= 2 && text.front() == L'"' && text.back() == L'"') {
        return trim(text.substr(1, text.size() - 2));
    }
    return text;
}

bool valid_names(const std::filesystem::path& folder) {
    std::wstring names = folder.relative_path().native();
    return names.find_first_of(NameCharactersNotAllowed) == std::wstring::npos && std::ranges::none_of(names, [](wchar_t c) { return c < L' '; });
}

}

std::optional<std::filesystem::path> parse_library_folder(std::wstring_view text) {
    std::filesystem::path folder(unquote(trim(text)));
    // A full path has both a drive (or server and share) and the root after it: D:\Music, not D:Music or \Music
    if (!folder.has_root_name() || !folder.has_root_directory() || !valid_names(folder)) {
        return std::nullopt;
    }
    folder = folder.lexically_normal();
    // D:\Music\ is D:\Music, but D:\ stays D:\.
    if (folder.has_relative_path() && !folder.has_filename()) {
        folder = folder.parent_path();
    }
    return folder;
}

}
