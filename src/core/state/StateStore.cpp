#include "core/state/StateStore.h"

#include <chrono>
#include <format>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

#include <glaze/glaze.hpp>

#include "core/json/IsoTime.h"
#include "core/json/JsonOptions.h"
#include "core/json/Utf8Path.h"

namespace westie::core {

namespace {

constexpr int StateVersion = 1;

struct PlaylistJson {
    std::string url{};
    std::string name{};
    std::string added_at{};
};

// state.json as it is on disk: camelCase keys and ISO 8601 times
struct StateJson {
    int version = StateVersion;
    std::map<std::string, PlaylistJson> playlists{};
};

std::filesystem::path backup_for(const std::filesystem::path& file) {
    std::filesystem::path backup = file;
    backup += L".bak";
    return backup;
}

}

}

template<>
struct glz::meta<westie::core::PlaylistJson> {
    using T = westie::core::PlaylistJson;
    static constexpr auto value = glz::object("url", &T::url, "name", &T::name, "addedAt", &T::added_at);
};

template<>
struct glz::meta<westie::core::StateJson> {
    using T = westie::core::StateJson;
    static constexpr auto value = glz::object("version", &T::version, "playlists", &T::playlists);
};

namespace westie::core {

StateStore::StateStore(platform::FileOps& files, std::filesystem::path file) :
        files_(files), file_(std::move(file)), backup_(backup_for(file_)), state_(load()) {}

const State& StateStore::state() const noexcept {
    return state_;
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
        // A time that doesn't read puts the playlist first rather than losing it
        std::chrono::system_clock::time_point added_at = parse_iso_time(playlist.added_at).value_or(std::chrono::system_clock::time_point{});
        state.playlists[key] = {.url = std::move(playlist.url), .name = std::move(playlist.name), .added_at = added_at};
    }
    return state;
}

void StateStore::save(const State& state) {
    StateJson json;
    for (const auto& [key, playlist] : state.playlists) {
        json.playlists[key] = {.url = playlist.url, .name = playlist.name, .added_at = format_iso_time(playlist.added_at)};
    }

    glz::expected<std::string, glz::error_ctx> contents = glz::write<PrettyJsonWriteOptions>(json);
    if (!contents) {
        throw std::runtime_error(std::format("Couldn't write {}: {}", to_utf8(file_), glz::format_error(contents.error())));
    }
    files_.replace_file_keeping_backup(file_, backup_, *contents + '\n');
}

}
