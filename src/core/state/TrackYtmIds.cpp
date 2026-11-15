#include "core/state/TrackYtmIds.h"

#include <algorithm>
#include <optional>
#include <ranges>
#include <string>
#include <utility>

#include "core/sources/SourceType.h"
#include "core/state/MatchReview.h"

namespace westie::core {

const std::string* first_source_of(const Track& track, SourceType type) {
    auto found = std::ranges::find(track.sources, std::optional(type), [](const std::string& key) { return source_type_of_key(key); });
    return found == track.sources.end() ? nullptr : &*found;
}

std::optional<std::string> own_ytm_id(std::string_view source_key) {
    if (source_type_of_key(source_key) != SourceType::YouTubeMusic) {
        return std::nullopt;
    }
    return std::string(source_key.substr(source_prefix(SourceType::YouTubeMusic).size() + 1));
}

std::optional<ResolvedYtmId> resolved_ytm_id(const State& state, const Track& track) {
    if (!track.ytm_override.empty()) {
        return ResolvedYtmId{.ytm_id = track.ytm_override, .source = track.sources.empty() ? "" : track.sources.front(), .overridden = true};
    }
    // The playlist owner chose a YouTube Music source
    if (const std::string* key = first_source_of(track, SourceType::YouTubeMusic)) {
        return ResolvedYtmId{.ytm_id = own_ytm_id(*key).value_or(""), .source = *key};
    }
    const std::string* key = first_source_of(track, SourceType::Spotify);
    if (key == nullptr) {
        return std::nullopt;
    }
    auto source = state.sources.find(*key);
    if (source == state.sources.end() || !source->second.match) {
        return std::nullopt;
    }
    return ResolvedYtmId{.ytm_id = source->second.match->ytm_id, .source = *key};
}

std::optional<std::string> track_with_ytm_id(const State& state, std::string_view ytm_id, std::string_view except) {
    for (const auto& [key, track] : state.tracks) {
        // A match that may be wrong takes no other track's sources in
        if (key == except || needs_review(state, track)) {
            continue;
        }
        if (std::optional<ResolvedYtmId> resolved = resolved_ytm_id(state, track); resolved && resolved->ytm_id == ytm_id) {
            return key;
        }
    }
    return std::nullopt;
}

std::set<std::string> track_ytm_ids(const State& state) {
    std::set<std::string> ytm_ids;
    for (const Track& track : state.tracks | std::views::values) {
        if (needs_review(state, track)) {
            continue;
        }
        if (std::optional<ResolvedYtmId> resolved = resolved_ytm_id(state, track)) {
            ytm_ids.insert(std::move(resolved->ytm_id));
        }
    }
    return ytm_ids;
}

}
