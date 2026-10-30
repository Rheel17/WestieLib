#include "core/matching/NameMatch.h"

#include <algorithm>
#include <cstddef>
#include <string_view>

#include "core/matching/MatchWords.h"

namespace westie::core {

namespace {

constexpr std::string_view Article = "the";
// Letters a name needs before one that starts with it, or holds it, counts: Tora starts ToraMusic, while shorter names
// start or sit in too many others
constexpr std::size_t LeastLetters = 4;

std::span<const std::string> without_article(std::span<const std::string> name) {
    return name.size() > 1 && name.front() == Article ? name.subspan(1) : name;
}

bool holds(std::span<const std::string> name, std::span<const std::string> other) {
    return !std::ranges::search(name, other).empty();
}

}

NameMatch name_match(std::span<const std::string> a, std::span<const std::string> b) {
    a = without_article(a);
    b = without_article(b);
    std::string a_letters = letters_of(a);
    std::string b_letters = letters_of(b);
    if (a_letters.empty() || b_letters.empty()) {
        return NameMatch::Other;
    }
    if (a_letters == b_letters) {
        return NameMatch::Same;
    }
    const std::string& shorter = a_letters.size() <= b_letters.size() ? a_letters : b_letters;
    const std::string& longer = a_letters.size() <= b_letters.size() ? b_letters : a_letters;
    // By its words, as Prince starts "Prince & The Revolution", or by its letters, as Tora starts ToraMusic
    if (std::ranges::starts_with(a, b) || std::ranges::starts_with(b, a) || (shorter.size() >= LeastLetters && longer.starts_with(shorter))) {
        return NameMatch::Starts;
    }
    // As Lauryn Hill in Ms. Lauryn Hill, but also as Phil Collins in The Hits Of Phil Collins
    if (shorter.size() >= LeastLetters && (holds(a, b) || holds(b, a))) {
        return NameMatch::Inside;
    }
    return NameMatch::Other;
}

}
