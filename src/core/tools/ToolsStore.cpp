#include "core/tools/ToolsStore.h"

#include <format>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <glaze/glaze.hpp>

#include "core/json/IsoTime.h"
#include "core/json/JsonOptions.h"
#include "core/json/Utf8Path.h"

namespace westie::core {

namespace {

constexpr int ToolsVersion = 1;

struct ToolJson {
    std::string release{};
    std::string digest{};
};

// tools.json as it is on disk: the tools keyed by name, so it can hold tools this version doesn't know
struct ToolsJson {
    int version = ToolsVersion;
    std::optional<std::string> checked_at{};
    std::map<std::string, ToolJson> tools{};
};

}

}

template<>
struct glz::meta<westie::core::ToolsJson> {
    using T = westie::core::ToolsJson;
    constexpr static auto value = glz::object("version", &T::version, "checkedAt", &T::checked_at, "tools", &T::tools);
};

namespace westie::core {

ToolsStore::ToolsStore(platform::FileOps& files, std::filesystem::path file) : files_(files), file_(std::move(file)) {}

InstalledTools ToolsStore::load() const {
    std::optional<std::string> contents = files_.read_file(file_);
    if (!contents) {
        return {};
    }

    // Unlike settings, nothing in it is the user's: a file that doesn't read means the tools get installed again
    ToolsJson json;
    if (glz::read<JsonReadOptions>(json, *contents)) {
        return {};
    }

    InstalledTools installed;
    for (auto& [name, tool] : json.tools) {
        if (std::optional<ToolPackage> package = tool_package_named(name)) {
            installed.tools[*package] = {.release = std::move(tool.release), .digest = std::move(tool.digest)};
        }
    }
    if (json.checked_at) {
        installed.checked_at = parse_iso_time(*json.checked_at);
    }
    return installed;
}

void ToolsStore::save(const InstalledTools& installed) {
    ToolsJson json;
    if (installed.checked_at) {
        json.checked_at = format_iso_time(*installed.checked_at);
    }
    for (const auto& [package, tool] : installed.tools) {
        json.tools[std::string(tool_package_name(package))] = {.release = tool.release, .digest = tool.digest};
    }

    glz::expected<std::string, glz::error_ctx> contents = glz::write<PrettyJsonWriteOptions>(json);
    if (!contents) {
        throw std::runtime_error(std::format("Couldn't write {}: {}", to_utf8(file_), glz::format_error(contents.error())));
    }
    files_.replace_file(file_, *contents + '\n');
}

}
