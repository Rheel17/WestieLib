#include "core/state/StateStore.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <glaze/glaze.hpp>

#include "core/json/IsoTime.h"
#include "core/json/JsonOptions.h"
#include "core/json/Utf8Path.h"

namespace westie::core {

namespace {

constexpr int StateVersion = 1;

struct StatusName {
    TrackStatus status = TrackStatus::Pending;
    std::string_view name{};
};

constexpr std::array StatusNames = {
    StatusName{TrackStatus::Pending, "pending"},   StatusName{TrackStatus::Downloaded, "downloaded"}, StatusName{TrackStatus::Failed, "failed"},
    StatusName{TrackStatus::Orphaned, "orphaned"}, StatusName{TrackStatus::Kept, "kept"},
};

struct PlaylistJson {
    std::string url{};
    std::string name{};
    std::string added_at{};
    std::optional<std::string> last_sync{};
    std::optional<std::string> fetch_error{};
    std::optional<std::string> fetch_error_at{};
    std::vector<std::string> sources{};
};

struct FailureName {
    TrackFailure failure = TrackFailure::None;
    std::string_view name{};
};

constexpr std::array FailureNames = {
    FailureName{TrackFailure::NoMatch, "noMatch"}, FailureName{TrackFailure::Search, "search"}, FailureName{TrackFailure::Download, "download"}
};

struct DoubtName {
    MatchDoubt doubt = MatchDoubt::Title;
    std::string_view name{};
};

constexpr std::array DoubtNames = {
    DoubtName{MatchDoubt::Title, "title"},   DoubtName{MatchDoubt::Artist, "artist"},     DoubtName{MatchDoubt::Version, "version"},
    DoubtName{MatchDoubt::Length, "length"}, DoubtName{MatchDoubt::Explicit, "explicit"},
};

struct MatchJson {
    std::string ytm_id{};
    double confidence = 0.0;
    std::optional<bool> confirmed{};
    std::optional<std::vector<std::string>> doubts{};
    std::string title{};
    std::vector<std::string> artists{};
    std::optional<std::string> album{};
    std::int64_t duration_ms = 0;
    std::optional<bool> explicit_content{};
    // Read from files of earlier versions, never written
    std::optional<std::string> old_ytm_id{};
};

struct SourceJson {
    std::string title{};
    std::vector<std::string> artists{};
    std::optional<std::string> album{};
    std::optional<int> year{};
    std::int64_t duration_ms = 0;
    std::optional<bool> explicit_content{};
    std::optional<std::string> cover_url{};
    std::optional<MatchJson> match{};
    std::string track{};
};

struct TrackJson {
    std::vector<std::string> sources{};
    std::optional<std::string> ytm_id{};
    std::optional<std::string> ytm_override{};
    std::string status{};
    std::optional<std::string> failure{};
    std::optional<std::string> error{};
    std::optional<std::string> file{};
    std::string added_at{};
    // Read from files of earlier versions, never written
    std::optional<std::string> old_ytm_override{};
};

// state.json as it is on disk: camelCase keys and ISO 8601 times
struct StateJson {
    int version = StateVersion;
    std::map<std::string, PlaylistJson> playlists{};
    std::map<std::string, SourceJson> sources{};
    std::map<std::string, TrackJson> tracks{};
    std::optional<std::set<std::string>> unavailable_ytm_ids{};
};

std::filesystem::path backup_for(const std::filesystem::path& file) {
    std::filesystem::path backup = file;
    backup += L".bak";
    return backup;
}

std::string_view status_name(TrackStatus status) {
    return std::ranges::find(StatusNames, status, &StatusName::status)->name;
}

TrackStatus status_named(std::string_view name) {
    auto found = std::ranges::find(StatusNames, name, &StatusName::name);
    // A status from a later version makes the track wait for another try rather than losing it
    return found == StatusNames.end() ? TrackStatus::Pending : found->status;
}

std::optional<std::string> failure_name(TrackFailure failure) {
    auto found = std::ranges::find(FailureNames, failure, &FailureName::failure);
    return found == FailureNames.end() ? std::nullopt : std::optional<std::string>(found->name);
}

TrackFailure failure_named(const std::optional<std::string>& name) {
    auto found = std::ranges::find(FailureNames, name.value_or(""), &FailureName::name);
    return found == FailureNames.end() ? TrackFailure::None : found->failure;
}

std::optional<std::string> optional_text(const std::string& text) {
    return text.empty() ? std::nullopt : std::optional(text);
}

std::optional<std::chrono::system_clock::time_point> parse_optional_time(const std::optional<std::string>& text) {
    return text ? parse_iso_time(*text) : std::nullopt;
}

std::optional<std::string> format_optional_time(const std::optional<std::chrono::system_clock::time_point>& time) {
    return time ? std::optional(format_iso_time(*time)) : std::nullopt;
}

Playlist playlist_of(PlaylistJson&& json) {
    return {
        .url = std::move(json.url),
        .name = std::move(json.name),
        // A time that doesn't read puts the playlist first rather than losing it
        .added_at = parse_iso_time(json.added_at).value_or(std::chrono::system_clock::time_point{}),
        .sources = std::move(json.sources),
        .last_sync = parse_optional_time(json.last_sync),
        .fetch_error = std::move(json.fetch_error).value_or(""),
        .fetch_error_at = parse_optional_time(json.fetch_error_at),
    };
}

PlaylistJson json_of(const Playlist& playlist) {
    return {
        .url = playlist.url,
        .name = playlist.name,
        .added_at = format_iso_time(playlist.added_at),
        .last_sync = format_optional_time(playlist.last_sync),
        .fetch_error = optional_text(playlist.fetch_error),
        .fetch_error_at = format_optional_time(playlist.fetch_error_at),
        .sources = playlist.sources,
    };
}

std::vector<MatchDoubt> doubts_named(const std::optional<std::vector<std::string>>& names) {
    std::vector<MatchDoubt> doubts;
    for (const std::string& name : names.value_or(std::vector<std::string>())) {
        // A doubt from a later version is left out; the next sync judges the match again anyway
        if (auto found = std::ranges::find(DoubtNames, name, &DoubtName::name); found != DoubtNames.end()) {
            doubts.push_back(found->doubt);
        }
    }
    return doubts;
}

std::optional<std::vector<std::string>> doubt_names(const std::vector<MatchDoubt>& doubts) {
    if (doubts.empty()) {
        return std::nullopt;
    }
    std::vector<std::string> names;
    for (MatchDoubt doubt : doubts) {
        names.emplace_back(std::ranges::find(DoubtNames, doubt, &DoubtName::doubt)->name);
    }
    return names;
}

Match match_of(MatchJson&& json) {
    return {
        .ytm_id = json.ytm_id.empty() ? std::move(json.old_ytm_id).value_or("") : std::move(json.ytm_id),
        .confidence = json.confidence,
        .confirmed = json.confirmed.value_or(false),
        .doubts = doubts_named(json.doubts),
        .title = std::move(json.title),
        .artists = std::move(json.artists),
        .album = std::move(json.album).value_or(""),
        .duration = std::chrono::milliseconds(json.duration_ms),
        .explicit_content = json.explicit_content,
    };
}

MatchJson json_of(const Match& match) {
    return {
        .ytm_id = match.ytm_id,
        .confidence = match.confidence,
        .confirmed = match.confirmed ? std::optional(true) : std::nullopt,
        .doubts = doubt_names(match.doubts),
        .title = match.title,
        .artists = match.artists,
        .album = optional_text(match.album),
        .duration_ms = match.duration.count(),
        .explicit_content = match.explicit_content,
    };
}

}

}

template<>
struct glz::meta<westie::core::PlaylistJson> {
    using T = westie::core::PlaylistJson;
    constexpr static auto value = glz::object(
            "url", &T::url, "name", &T::name, "addedAt", &T::added_at, "lastSync", &T::last_sync, "fetchError", &T::fetch_error, "fetchErrorAt",
            &T::fetch_error_at, "sources", &T::sources
    );
};

template<>
struct glz::meta<westie::core::MatchJson> {
    using T = westie::core::MatchJson;
    constexpr static auto value = glz::object(
            "ytmId", &T::ytm_id, "confidence", &T::confidence, "confirmed", &T::confirmed, "doubts", &T::doubts, "title", &T::title, "artists",
            &T::artists, "album", &T::album, "durationMs", &T::duration_ms, "explicit", &T::explicit_content, "videoId", &T::old_ytm_id
    );
};

template<>
struct glz::meta<westie::core::SourceJson> {
    using T = westie::core::SourceJson;
    constexpr static auto value = glz::object(
            "title", &T::title, "artists", &T::artists, "album", &T::album, "year", &T::year, "durationMs", &T::duration_ms, "explicit",
            &T::explicit_content, "coverUrl", &T::cover_url, "match", &T::match, "track", &T::track
    );
};

template<>
struct glz::meta<westie::core::TrackJson> {
    using T = westie::core::TrackJson;
    constexpr static auto value = glz::object(
            "sources", &T::sources, "ytmId", &T::ytm_id, "ytmOverride", &T::ytm_override, "status", &T::status, "failure", &T::failure, "error",
            &T::error, "file", &T::file, "addedAt", &T::added_at, "videoOverride", &T::old_ytm_override
    );
};

template<>
struct glz::meta<westie::core::StateJson> {
    using T = westie::core::StateJson;
    constexpr static auto value = glz::object(
            "version", &T::version, "playlists", &T::playlists, "sources", &T::sources, "tracks", &T::tracks, "unavailableYtmIds", &T::unavailable_ytm_ids
    );
};

namespace westie::core {

StateStore::StateStore(platform::FileOps& files, std::filesystem::path file) :
        files_(files), file_(std::move(file)), backup_(backup_for(file_)), state_(std::make_shared<const State>(load())) {}

std::shared_ptr<const State> StateStore::snapshot() const noexcept {
    return state_.load();
}

State StateStore::load() const {
    std::optional<std::string> contents = files_.read_file(file_);
    if (!contents) {
        return {};
    }

    // The state holds what the user did, so a file that doesn't read is the user's to fix, never to overwrite
    StateJson json;
    if (glz::error_ctx error = glz::read<JsonReadOptions>(json, *contents)) {
        throw std::runtime_error(
                std::format(
                        "Couldn't read {}. Fix it, or replace it with {}, then start WestieLib again.\n\n{}", to_utf8(file_), to_utf8(backup_),
                        glz::format_error(error, *contents)
                )
        );
    }

    State state;
    for (auto& [key, playlist] : json.playlists) {
        state.playlists[key] = playlist_of(std::move(playlist));
    }
    for (auto& [key, source] : json.sources) {
        state.sources[key] = {
            .title = std::move(source.title),
            .artists = std::move(source.artists),
            .album = std::move(source.album).value_or(""),
            .year = source.year,
            .duration = std::chrono::milliseconds(source.duration_ms),
            .explicit_content = source.explicit_content,
            .cover_url = std::move(source.cover_url).value_or(""),
            .match = source.match ? std::optional(match_of(std::move(*source.match))) : std::nullopt,
            .track = std::move(source.track),
        };
    }
    for (auto& [key, track] : json.tracks) {
        state.tracks[key] = {
            .sources = std::move(track.sources),
            .ytm_id = std::move(track.ytm_id).value_or(""),
            .ytm_override = std::move(track.ytm_override).or_else([&] { return std::move(track.old_ytm_override); }).value_or(""),
            .status = status_named(track.status),
            .failure = failure_named(track.failure),
            .error = std::move(track.error).value_or(""),
            .file = track.file ? path_from_utf8(*track.file) : std::filesystem::path(),
            .added_at = parse_iso_time(track.added_at).value_or(std::chrono::system_clock::time_point{}),
        };
    }
    state.unavailable_ytm_ids = std::move(json.unavailable_ytm_ids).value_or(std::set<std::string>());
    return state;
}

void StateStore::save(const State& state) {
    StateJson json;
    for (const auto& [key, playlist] : state.playlists) {
        json.playlists[key] = json_of(playlist);
    }
    for (const auto& [key, source] : state.sources) {
        json.sources[key] = {
            .title = source.title,
            .artists = source.artists,
            .album = optional_text(source.album),
            .year = source.year,
            .duration_ms = source.duration.count(),
            .explicit_content = source.explicit_content,
            .cover_url = optional_text(source.cover_url),
            .match = source.match ? std::optional(json_of(*source.match)) : std::nullopt,
            .track = source.track,
        };
    }
    for (const auto& [key, track] : state.tracks) {
        json.tracks[key] = {
            .sources = track.sources,
            .ytm_id = optional_text(track.ytm_id),
            .ytm_override = optional_text(track.ytm_override),
            .status = std::string(status_name(track.status)),
            .failure = failure_name(track.failure),
            .error = optional_text(track.error),
            .file = track.file.empty() ? std::nullopt : std::optional(to_utf8(track.file)),
            .added_at = format_iso_time(track.added_at),
        };
    }
    if (!state.unavailable_ytm_ids.empty()) {
        json.unavailable_ytm_ids = state.unavailable_ytm_ids;
    }

    glz::expected<std::string, glz::error_ctx> contents = glz::write<PrettyJsonWriteOptions>(json);
    if (!contents) {
        throw std::runtime_error(std::format("Couldn't write {}: {}", to_utf8(file_), glz::format_error(contents.error())));
    }
    files_.replace_file_keeping_backup(file_, backup_, *contents + '\n');
}

}
