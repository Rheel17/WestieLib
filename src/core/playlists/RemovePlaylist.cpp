#include "core/playlists/RemovePlaylist.h"

namespace westie::core {

bool remove_playlist(StateStore& store, const std::string& key) {
    if (!store.state().playlists.contains(key)) {
        return false;
    }
    // Tracks don't exist yet; once they do, the ones left in no playlist become orphaned here
    store.update([&](State& state) { state.playlists.erase(key); });
    return true;
}

}
