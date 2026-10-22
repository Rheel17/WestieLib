#pragma once

#include <string_view>

namespace westie::core {

enum class SourceType { YouTubeMusic, Spotify };

std::string_view source_prefix(SourceType type) noexcept;

}
