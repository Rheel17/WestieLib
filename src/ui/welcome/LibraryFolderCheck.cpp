#include "ui/welcome/LibraryFolderCheck.h"

#include <format>
#include <utility>

#include "core/library/WindowsNames.h"
#include "platform/win32/PathProbe.h"
#include "platform/win32/Utf16.h"
#include "ui/welcome/LibraryFolderText.h"

namespace westie::ui {

namespace {

// Windows would create another folder than the one the text names, or none at all
FolderHint name_hint(const std::filesystem::path& folder) {
    for (const std::filesystem::path& name : folder.relative_path()) {
        switch (core::windows_name_problem(platform::win32::to_utf8(name.native()))) {
        case core::NameProblem::EndsInSpaceOrPeriod: return FolderHint::EndsInSpaceOrPeriod;
        case core::NameProblem::ReservedName:        return FolderHint::ReservedName;
        case core::NameProblem::None:                break;
        }
    }
    return FolderHint::None;
}

}

LibraryFolderCheck check_library_folder(std::wstring_view text) {
    std::optional<std::filesystem::path> folder = parse_library_folder(text);
    if (!folder) {
        return {.hint = FolderHint::NotFullPath};
    }
    if (FolderHint hint = name_hint(*folder); hint != FolderHint::None) {
        return {.hint = hint};
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
    case FolderHint::NotFullPath:         return std::format(L"Enter a full folder path, such as {}.", default_folder.native());
    case FolderHint::EndsInSpaceOrPeriod: return L"A folder name can't end in a space or a period.";
    case FolderHint::ReservedName:        return L"CON, NUL, COM1 and the like are names Windows keeps for devices.";
    case FolderHint::NoSuchDrive:         return L"That drive isn't on this PC.";
    case FolderHint::NotAFolder:          return L"That's a file, not a folder.";
    case FolderHint::WillBeCreated:       return L"This folder doesn't exist yet. The first update creates it.";
    default:                              return {};
    }
}

bool is_problem(FolderHint hint) noexcept {
    return hint != FolderHint::None && hint != FolderHint::WillBeCreated;
}

}
