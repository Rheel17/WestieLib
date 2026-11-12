#include "core/log/LogSubject.h"

#include "core/state/EffectiveMetadata.h"

namespace westie::core {

LogSubject playlist_subject(const State& state, const std::string& playlist) {
    auto found = state.playlists.find(playlist);
    return {.playlist = playlist, .name = found == state.playlists.end() ? std::string() : found->second.name};
}

LogSubject track_subject(const State& state, const std::string& track) {
    auto found = state.tracks.find(track);
    return {.track = track, .name = found == state.tracks.end() ? std::string() : effective_metadata(state, found->second).title};
}

}
