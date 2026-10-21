#include "core/SettingsStore.h"

#include <cstdint>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <glaze/glaze.hpp>

namespace westie::core {

namespace {

constexpr int SettingsVersion = 1;

// settings.json as it is on disk: camelCase keys, and the paths in UTF-8
struct SettingsJson {
    int version = SettingsVersion;
    std::string library_path{};
};

// Files from a later version may have keys this one doesn't know
constexpr glz::opts ReadOptions{.error_on_unknown_keys = false};

struct WriteOptions : glz::opts {
    std::uint8_t indentation_width = 4;
};

constexpr WriteOptions PrettyWriteOptions = [] {
    WriteOptions options;
    options.prettify = true;
    return options;
}();

std::string to_utf8(const std::filesystem::path& path) {
    std::u8string text = path.u8string();
    return {text.begin(), text.end()};
}

std::filesystem::path from_utf8(std::string_view text) {
    return std::u8string(text.begin(), text.end());
}

}

}

template<>
struct glz::meta<westie::core::SettingsJson> {
    static constexpr auto value =
            glz::object("version", &westie::core::SettingsJson::version, "libraryPath", &westie::core::SettingsJson::library_path);
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
    if (glz::error_ctx error = glz::read<ReadOptions>(json, *contents)) {
        throw std::runtime_error(
                std::format("Couldn't read {}. Fix or delete it, then start WestieLib again.\n\n{}", to_utf8(file_), glz::format_error(error, *contents))
        );
    }

    Settings settings = defaults_;
    if (!json.library_path.empty()) {
        settings.library_path = from_utf8(json.library_path);
    }
    return settings;
}

void SettingsStore::save(const Settings& settings) {
    SettingsJson json{.library_path = to_utf8(settings.library_path)};
    glz::expected<std::string, glz::error_ctx> contents = glz::write<PrettyWriteOptions>(json);
    if (!contents) {
        throw std::runtime_error(std::format("Couldn't write {}: {}", to_utf8(file_), glz::format_error(contents.error())));
    }
    files_.replace_file(file_, *contents + '\n');
}

}
