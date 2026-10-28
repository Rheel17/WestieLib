#include "core/state/ListedSources.h"

#include <ranges>

namespace westie::core {

std::set<std::string> listed_sources(const State& state) {
    std::set<std::string> listed;
    for (const Playlist& playlist : state.playlists | std::views::values) {
        listed.insert(playlist.sources.begin(), playlist.sources.end());
    }
    return listed;
}

}
