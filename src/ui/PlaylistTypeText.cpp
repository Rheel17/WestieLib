#include "ui/PlaylistTypeText.h"

namespace westie::ui {

std::wstring playlist_type_text(core::SourceType type) {
    switch (type) {
    case core::SourceType::YouTubeMusic: return L"YouTube Music playlist";
    case core::SourceType::Spotify:      return L"Spotify playlist";
    }
    return {};
}

}
