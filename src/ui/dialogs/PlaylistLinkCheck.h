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
    // For a playlist in the list already; empty when it was stored without one
    std::string added_name{};
};

// The name a playlist has in the list, or nothing when it isn't in it
using AddedPlaylistName = std::function<std::optional<std::string>(const core::PlaylistLink&)>;

PlaylistLinkCheck check_playlist_link(std::wstring_view text, const AddedPlaylistName& added_name);

}
