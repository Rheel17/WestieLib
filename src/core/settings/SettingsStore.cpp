#include "core/settings/SettingsStore.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <glaze/glaze.hpp>

#include "core/json/JsonOptions.h"
#include "core/json/Utf8Path.h"

namespace westie::core {

namespace {

constexpr int SettingsVersion = 1;

struct FormatName {
    AudioFormat format = AudioFormat::Opus;
    std::string_view name{};
};

constexpr std::array FormatNames = {FormatName{AudioFormat::Opus, "opus"}, FormatName{AudioFormat::Flac, "flac"}};

// settings.json as it is on disk: camelCase keys, and the paths in UTF-8. Missing keys keep their defaults.
struct SettingsJson {
    int version = SettingsVersion;
    std::string library_path{};
    std::optional<std::string> format{};
    std::optional<bool> embed_cover{};
    std::optional<int> concurrency{};
    std::optional<std::int64_t> delay_between_tracks_seconds{};
    std::optional<bool> auto_update_tools{};
    std::optional<bool> open_choose_match{};
};

AudioFormat format_named(const std::optional<std::string>& name, AudioFormat fallback) {
    if (!name) {
        return fallback;
    }
    auto found = std::ranges::find(FormatNames, *name, &FormatName::name);
    // A format from a later version downloads as the default rather than not at all
    return found == FormatNames.end() ? AudioFormat::Opus : found->format;
}

std::string format_name(AudioFormat format) {
    return std::string(std::ranges::find(FormatNames, format, &FormatName::format)->name);
}

}

}

template<>
struct glz::meta<westie::core::SettingsJson> {
    using T = westie::core::SettingsJson;
    constexpr static auto value = glz::object(
            "version", &T::version, "libraryPath", &T::library_path, "format", &T::format, "embedCover", &T::embed_cover, "concurrency", &T::concurrency,
            "delayBetweenTracksSeconds", &T::delay_between_tracks_seconds, "autoUpdateTools", &T::auto_update_tools, "openChooseMatch", &T::open_choose_match
    );
};

namespace westie::core {

SettingsStore::SettingsStore(platform::FileOps& files, std::filesystem::path file, Settings defaults) :
        files_(files), file_(std::move(file)), defaults_(std::move(defaults)), current_(defaults_) {}

const Settings& SettingsStore::defaults() const noexcept {
    return defaults_;
}

Settings SettingsStore::load() {
    std::optional<std::string> contents = files_.read_file(file_);
    if (!contents) {
        return current();
    }

    // A file that doesn't read is the user's to fix; writing the defaults over it would lose it
    SettingsJson json;
    if (glz::error_ctx error = glz::read<JsonReadOptions>(json, *contents)) {
        throw std::runtime_error(
                std::format("Couldn't read {}. Fix or delete it, then start WestieLib again.\n\n{}", to_utf8(file_), glz::format_error(error, *contents))
        );
    }

    Settings settings = defaults_;
    if (!json.library_path.empty()) {
        settings.library_path = path_from_utf8(json.library_path);
    }
    settings.format = format_named(json.format, defaults_.format);
    settings.embed_cover = json.embed_cover.value_or(defaults_.embed_cover);
    settings.concurrency = std::clamp(json.concurrency.value_or(defaults_.concurrency), MinConcurrency, MaxConcurrency);
    if (json.delay_between_tracks_seconds) {
        settings.delay_between_tracks = std::chrono::seconds(std::max<std::int64_t>(*json.delay_between_tracks_seconds, 0));
    }
    settings.auto_update_tools = json.auto_update_tools.value_or(defaults_.auto_update_tools);
    settings.open_choose_match = json.open_choose_match.value_or(defaults_.open_choose_match);

    std::scoped_lock lock(mutex_);
    current_ = settings;
    return settings;
}

void SettingsStore::save(const Settings& settings) {
    SettingsJson json{
        .library_path = to_utf8(settings.library_path),
        .format = format_name(settings.format),
        .embed_cover = settings.embed_cover,
        .concurrency = settings.concurrency,
        .delay_between_tracks_seconds = settings.delay_between_tracks.count(),
        .auto_update_tools = settings.auto_update_tools,
        .open_choose_match = settings.open_choose_match,
    };
    glz::expected<std::string, glz::error_ctx> contents = glz::write<PrettyJsonWriteOptions>(json);
    if (!contents) {
        throw std::runtime_error(std::format("Couldn't write {}: {}", to_utf8(file_), glz::format_error(contents.error())));
    }
    files_.replace_file(file_, *contents + '\n');

    std::scoped_lock lock(mutex_);
    current_ = settings;
}

Settings SettingsStore::current() const {
    std::scoped_lock lock(mutex_);
    return current_;
}

}
