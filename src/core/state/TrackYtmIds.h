#pragma once

#include <optional>
#include <set>
#include <string>
#include <string_view>

#include "core/sources/SourceType.h"
#include "core/state/State.h"

namespace westie::core {

struct ResolvedYtmId {
    std::string ytm_id{};
    // A YouTube Music source, whose own id it is, or the source whose match it is; for an override, the track's
    // first source
    std::string source{};
    bool overridden = false;
    bool matched = false;
};

const std::string* first_source_of(const Track& track, SourceType type);
const std::string* available_ytm_source_of(const State& state, const Track& track);
// A Spotify entry, or a YouTube Music entry whose own id YouTube Music no longer plays
bool is_matched_entry(const State& state, std::string_view source_key);
const std::string* match_source_of(const State& state, const Track& track);
const std::string* source_to_match(const State& state, const Track& track);
std::optional<std::string> own_ytm_id(std::string_view source_key);
std::optional<ResolvedYtmId> resolved_ytm_id(const State& state, const Track& track);
std::optional<std::string> track_with_ytm_id(const State& state, std::string_view ytm_id, std::string_view except = {});
std::set<std::string> track_ytm_ids(const State& state);

}
