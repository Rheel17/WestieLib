#include "core/playlists/RemovePlaylist.h"

#include <memory>
#include <utility>

#include "core/log/LogLevel.h"
#include "core/log/LogSubject.h"
#include "core/state/PruneTracks.h"

namespace westie::core {

bool remove_playlist(Library& library, Log& log, const std::string& key) {
    std::shared_ptr<const State> before = library.snapshot();
    if (!before->playlists.contains(key)) {
        return false;
    }
    library.update([&](State& state) {
        state.playlists.erase(key);
        prune_tracks(state);
    });
    log.write(LogLevel::Info, "Removed the playlist", playlist_subject(*before, key));
    return true;
}

}
