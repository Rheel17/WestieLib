#include "core/playlists/AddPlaylist.h"

#include <utility>

namespace westie::core {

bool add_playlist(StateStore& store, const PlaylistLink& link, std::string name, std::chrono::system_clock::time_point now) {
    std::string key = link.key();
    if (store.snapshot()->playlists.contains(key)) {
        return false;
    }
    store.update([&](State& state) { state.playlists[key] = {.url = link.url(), .name = std::move(name), .added_at = now}; });
    return true;
}

}
