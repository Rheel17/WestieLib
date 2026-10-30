#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "platform/TextNormalizer.h"

namespace westie::core {

struct TitleParts {
    // The text before the first " - ", outside brackets
    std::vector<std::string> name{};
    // Words before a ": " in the name, as Einaudi in "Einaudi: Experience"
    std::vector<std::string> lead{};
    // The name's last "with" and the words after it, as in "Retrograde with Glass House" and "Dance With Me"
    std::vector<std::string> with_tail{};
    // The brackets, the " - " suffixes and a feat. tail, split at ";" and before a feat or with; the ones that start
    // with "from" are left out, with the brackets they hold
    std::vector<std::vector<std::string>> parts{};
};

TitleParts title_parts(const platform::TextNormalizer& normalizer, std::string_view title);
std::vector<std::string> whole_name(const TitleParts& title);

}
