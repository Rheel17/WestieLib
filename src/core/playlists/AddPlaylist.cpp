#include "core/playlists/AddPlaylist.h"

#include <string>

namespace westie::core {

bool add_playlist(StateStore& store, const PlaylistLink& link, std::chrono::system_clock::time_point now) {
    std::string key = link.key();
    if (store.state().playlists.contains(key)) {
        return false;
    }
    // The name comes with the first fetch
    store.update([&](State& state) { state.playlists[key] = {.url = link.url(), .added_at = now}; });
    return true;
}

}
