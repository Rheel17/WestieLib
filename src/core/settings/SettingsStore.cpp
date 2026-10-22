#include "core/settings/SettingsStore.h"

#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <glaze/glaze.hpp>

#include "core/json/JsonOptions.h"
#include "core/json/Utf8Path.h"

namespace westie::core {

namespace {

constexpr int SettingsVersion = 1;

// settings.json as it is on disk: camelCase keys, and the paths in UTF-8. Missing keys keep their defaults.
struct SettingsJson {
    int version = SettingsVersion;
    std::string library_path{};
    std::optional<bool> auto_update_tools{};
};

}

}

template<>
struct glz::meta<westie::core::SettingsJson> {
    using T = westie::core::SettingsJson;
    constexpr static auto value = glz::object("version", &T::version, "libraryPath", &T::library_path, "autoUpdateTools", &T::auto_update_tools);
};

namespace westie::core {

SettingsStore::SettingsStore(platform::FileOps& files, std::filesystem::path file, Settings defaults) :
        files_(files), file_(std::move(file)), defaults_(std::move(defaults)) {}

const Settings& SettingsStore::defaults() const noexcept {
    return defaults_;
}

Settings SettingsStore::load() const {
    std::optional<std::string> contents = files_.read_file(file_);
    if (!contents) {
        return defaults_;
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
    settings.auto_update_tools = json.auto_update_tools.value_or(defaults_.auto_update_tools);
    return settings;
}

void SettingsStore::save(const Settings& settings) {
    SettingsJson json{.library_path = to_utf8(settings.library_path), .auto_update_tools = settings.auto_update_tools};
    glz::expected<std::string, glz::error_ctx> contents = glz::write<PrettyJsonWriteOptions>(json);
    if (!contents) {
        throw std::runtime_error(std::format("Couldn't write {}: {}", to_utf8(file_), glz::format_error(contents.error())));
    }
    files_.replace_file(file_, *contents + '\n');
}

}
