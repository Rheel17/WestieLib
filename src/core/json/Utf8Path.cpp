#include "core/json/Utf8Path.h"

namespace westie::core {

std::string to_utf8(const std::filesystem::path& path) {
    std::u8string text = path.u8string();
    return {text.begin(), text.end()};
}

std::filesystem::path path_from_utf8(std::string_view text) {
    return std::u8string(text.begin(), text.end());
}

}
