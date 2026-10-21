#include "ui/welcome/LibraryFolderCheck.h"

#include <format>
#include <utility>

#include "platform/win32/PathProbe.h"
#include "ui/welcome/LibraryFolderText.h"

namespace westie::ui {

LibraryFolderCheck check_library_folder(std::wstring_view text) {
    std::optional<std::filesystem::path> folder = parse_library_folder(text);
    if (!folder) {
        return {.hint = FolderHint::NotFullPath};
    }
    switch (platform::win32::probe_path(*folder)) {
    case platform::win32::PathKind::NoSuchDrive: return {.hint = FolderHint::NoSuchDrive};
    case platform::win32::PathKind::File:        return {.hint = FolderHint::NotAFolder};
    case platform::win32::PathKind::Nothing:     return {.folder = std::move(folder), .hint = FolderHint::WillBeCreated};
    default:                                     return {.folder = std::move(folder)};
    }
}

std::wstring hint_text(FolderHint hint, const std::filesystem::path& default_folder) {
    switch (hint) {
    case FolderHint::NotFullPath:   return std::format(L"Enter a full folder path, such as {}.", default_folder.native());
    case FolderHint::NoSuchDrive:   return L"That drive isn't on this PC.";
    case FolderHint::NotAFolder:    return L"That's a file, not a folder.";
    case FolderHint::WillBeCreated: return L"This folder doesn't exist yet. WestieLib creates it when it first syncs.";
    default:                        return {};
    }
}

bool is_problem(FolderHint hint) noexcept {
    return hint == FolderHint::NotFullPath || hint == FolderHint::NoSuchDrive || hint == FolderHint::NotAFolder;
}

}
