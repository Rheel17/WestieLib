#include "core/sources/SourceType.h"

namespace westie::core {

std::string_view source_prefix(SourceType type) noexcept {
    switch (type) {
    case SourceType::YouTubeMusic: return "ytm";
    case SourceType::Spotify:      return "spotify";
    }
    return {};
}

}
