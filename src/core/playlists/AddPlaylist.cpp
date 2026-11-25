#include "core/playlists/AddPlaylist.h"

#include <utility>

#include "core/log/LogLevel.h"
#include "core/log/LogSubject.h"

namespace westie::core {

bool add_playlist(Library& library, Log& log, const PlaylistLink& link, std::string name, std::chrono::system_clock::time_point now) {
    std::string key = link.key();
    if (library.snapshot()->playlists.contains(key)) {
        return false;
    }
    LogSubject subject{.playlist = key, .name = name};
    library.update([&](State& state) { state.playlists[key] = {.url = link.url(), .name = std::move(name), .added_at = now}; });
    log.write(LogLevel::Info, "Added the playlist", std::move(subject));
    return true;
}

}
