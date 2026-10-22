#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace westie::core {

enum class SourceType { YouTubeMusic, Spotify };

constexpr std::array AllSourceTypes = {SourceType::YouTubeMusic, SourceType::Spotify};

std::string_view source_prefix(SourceType type) noexcept;
std::optional<SourceType> source_type_of_key(std::string_view key) noexcept;

}
