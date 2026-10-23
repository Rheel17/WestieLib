#include "core/state/StateStore.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <format>
#include <map>
#include <optional>
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

struct SourceJson {
    std::string title{};
    std::vector<std::string> artists{};
    std::optional<std::string> album{};
    std::int64_t duration_ms = 0;
    std::string track{};
};

struct TrackJson {
    std::vector<std::string> sources{};
    std::string status{};
    std::string added_at{};
};

// state.json as it is on disk: camelCase keys and ISO 8601 times
struct StateJson {
    int version = StateVersion;
    std::map<std::string, PlaylistJson> playlists{};
    std::map<std::string, SourceJson> sources{};
    std::map<std::string, TrackJson> tracks{};
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
        .fetch_error = playlist.fetch_error.empty() ? std::nullopt : std::optional(playlist.fetch_error),
        .fetch_error_at = format_optional_time(playlist.fetch_error_at),
        .sources = playlist.sources,
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
struct glz::meta<westie::core::SourceJson> {
    using T = westie::core::SourceJson;
    constexpr static auto value = glz::object("title", &T::title, "artists", &T::artists, "album", &T::album, "durationMs", &T::duration_ms, "track", &T::track);
};

template<>
struct glz::meta<westie::core::TrackJson> {
    using T = westie::core::TrackJson;
    constexpr static auto value = glz::object("sources", &T::sources, "status", &T::status, "addedAt", &T::added_at);
};

template<>
struct glz::meta<westie::core::StateJson> {
    using T = westie::core::StateJson;
    constexpr static auto value = glz::object("version", &T::version, "playlists", &T::playlists, "sources", &T::sources, "tracks", &T::tracks);
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
            .duration = std::chrono::milliseconds(source.duration_ms),
            .track = std::move(source.track),
        };
    }
    for (auto& [key, track] : json.tracks) {
        state.tracks[key] = {
            .sources = std::move(track.sources),
            .status = status_named(track.status),
            .added_at = parse_iso_time(track.added_at).value_or(std::chrono::system_clock::time_point{}),
        };
    }
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
            .album = source.album.empty() ? std::nullopt : std::optional(source.album),
            .duration_ms = source.duration.count(),
            .track = source.track,
        };
    }
    for (const auto& [key, track] : state.tracks) {
        json.tracks[key] = {.sources = track.sources, .status = std::string(status_name(track.status)), .added_at = format_iso_time(track.added_at)};
    }

    glz::expected<std::string, glz::error_ctx> contents = glz::write<PrettyJsonWriteOptions>(json);
    if (!contents) {
        throw std::runtime_error(std::format("Couldn't write {}: {}", to_utf8(file_), glz::format_error(contents.error())));
    }
    files_.replace_file_keeping_backup(file_, backup_, *contents + '\n');
}

}
