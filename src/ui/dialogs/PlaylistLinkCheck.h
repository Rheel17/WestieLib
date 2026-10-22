#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "core/playlists/PlaylistLink.h"

namespace westie::ui {

enum class LinkState { Empty, NotSupported, AlreadyAdded, New };

struct PlaylistLinkCheck {
    std::optional<core::PlaylistLink> link{};
    LinkState state = LinkState::Empty;
};

PlaylistLinkCheck check_playlist_link(std::wstring_view text, const std::function<bool(const core::PlaylistLink&)>& is_added);
std::wstring link_check_text(const PlaylistLinkCheck& check);

}
