#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace westie::ui {

enum class FolderHint { None, NotFullPath, NoSuchDrive, NotAFolder, WillBeCreated };

constexpr std::array ShownFolderHints = {FolderHint::NotFullPath, FolderHint::NoSuchDrive, FolderHint::NotAFolder, FolderHint::WillBeCreated};

struct LibraryFolderCheck {
    std::optional<std::filesystem::path> folder{};
    FolderHint hint = FolderHint::None;
};

LibraryFolderCheck check_library_folder(std::wstring_view text);
std::wstring hint_text(FolderHint hint, const std::filesystem::path& default_folder);
bool is_problem(FolderHint hint) noexcept;

}
