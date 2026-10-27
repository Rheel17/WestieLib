#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "platform/TextNormalizer.h"

namespace westie::core {

struct MatchText {
    // Without bracketed parts, a " - " suffix or a feat. tail
    std::vector<std::string> words{};
    // Words such as live or remix, from anywhere in the text, sorted
    std::vector<std::string> versions{};
};

MatchText match_text(const platform::TextNormalizer& normalizer, std::string_view text);
std::string bare_title(std::string_view title);

}
