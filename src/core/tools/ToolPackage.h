#pragma once

#include <array>
#include <optional>
#include <span>
#include <string_view>

namespace westie::core {

enum class ToolPackage { YtDlp, Ffmpeg };

constexpr std::array AllToolPackages = {ToolPackage::YtDlp, ToolPackage::Ffmpeg};

std::string_view tool_package_name(ToolPackage package) noexcept;
std::optional<ToolPackage> tool_package_named(std::string_view name) noexcept;
std::span<const std::wstring_view> tool_package_files(ToolPackage package) noexcept;

}
