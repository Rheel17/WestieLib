#include "core/playlists/RemovePlaylist.h"

#include "core/state/PruneTracks.h"

namespace westie::core {

bool remove_playlist(StateStore& store, const std::string& key) {
    if (!store.snapshot()->playlists.contains(key)) {
        return false;
    }
    store.update([&](State& state) {
        state.playlists.erase(key);
        prune_tracks(state);
    });
    return true;
}

}
