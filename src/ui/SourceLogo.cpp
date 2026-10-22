#include "ui/SourceLogo.h"

#include "ResourceIds.h"

namespace westie::ui {

int source_logo_icon(core::SourceType type) noexcept {
    switch (type) {
    case core::SourceType::YouTubeMusic: return IDI_YTM;
    case core::SourceType::Spotify:      return IDI_SPOTIFY;
    }
    return 0;
}

}
