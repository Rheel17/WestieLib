#include "core/matching/Credits.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <string_view>
#include <utility>

#include "core/matching/MatchWords.h"

namespace westie::core {

namespace {

// Words between the names in a credit, in English and in a few languages titles often use. Cover is one because the
// artists show who performs; remix isn't, since a remixer is listed as one more artist
constexpr std::array<std::string_view, 20> Connectors = {
    "feat", "with", "x", "vs", "and", "of", "by", "prod", "produced", "the", "cover", "con", "y", "e", "et", "avec", "mit", "und", "cu", "met",
};
// The ones that separate a name from the next, as in "Macklemore & Patrick Stump of Fall Out Boy"
constexpr std::array<std::string_view, 14> ListConnectors = {"feat", "with", "x", "vs", "and", "con", "y", "e", "et", "avec", "mit", "und", "cu", "met"};
constexpr std::array<std::string_view, 2> CreditWords = {"feat", "with"};

bool led_by_credit_word(std::span<const std::string> part) {
    return !part.empty() && std::ranges::contains(CreditWords, part.front());
}

std::size_t longest_name_at(std::span<const std::string> words, std::size_t at, const std::vector<std::vector<std::string>>& known) {
    std::size_t longest = 0;
    for (const std::vector<std::string>& name : known) {
        // By its letters, as Glasshouse names Glass House
        std::string wanted = letters_of(name);
        std::string letters;
        for (std::size_t end = at; end < words.size() && !wanted.empty() && letters.size() < wanted.size(); ++end) {
            letters += words[end];
            if (letters == wanted) {
                longest = std::max(longest, end - at + 1);
            }
        }
    }
    return longest;
}

bool made_of_names(std::span<const std::string> part, const std::vector<std::vector<std::string>>& known) {
    bool named = false;
    std::size_t at = 0;
    while (at < part.size()) {
        if (std::size_t length = longest_name_at(part, at, known); length > 0) {
            at += length;
            named = true;
        } else if (std::ranges::contains(Connectors, part[at])) {
            ++at;
        } else {
            return false;
        }
    }
    return named;
}

bool starts_alike(std::span<const std::string> a, std::span<const std::string> b) {
    return !a.empty() && !b.empty() && (std::ranges::starts_with(a, b) || std::ranges::starts_with(b, a));
}

}

std::vector<std::vector<std::string>> name_pieces(std::span<const std::string> name) {
    std::vector<std::vector<std::string>> pieces;
    std::vector<std::string> piece;
    for (const std::string& word : name) {
        if (!std::ranges::contains(ListConnectors, word)) {
            piece.push_back(word);
        } else if (!piece.empty()) {
            pieces.push_back(std::exchange(piece, {}));
        }
    }
    if (!piece.empty()) {
        pieces.push_back(std::move(piece));
    }
    // A name that holds no other is no piece of itself
    if (pieces.size() < 2) {
        pieces.clear();
    }
    return pieces;
}

std::vector<std::vector<std::string>> credited_names(const TitleParts& title) {
    std::vector<std::vector<std::string>> names;
    auto add = [&](std::span<const std::string> credit) {
        // The credit as a whole, and each name in it, as "Patrick Stump of Fall Out Boy" in "Macklemore & Patrick Stump
        // of Fall Out Boy"
        names.emplace_back(credit.begin(), credit.end());
        std::ranges::move(name_pieces(credit), std::back_inserter(names));
    };
    for (const std::vector<std::string>& part : title.parts) {
        if (led_by_credit_word(part)) {
            add(std::span(part).subspan(1));
        }
    }
    if (!title.with_tail.empty()) {
        add(std::span(title.with_tail).subspan(1));
    }
    return names;
}

bool is_credit(std::span<const std::string> part, const std::vector<std::vector<std::string>>& known) {
    if (made_of_names(part, known)) {
        return true;
    }
    // A guest named more fully or more briefly than the other song names them, as "Count Basie And His Orchestra" for
    // Count Basie, or Gary Lightbody for "Gary Lightbody of Snow Patrol"
    return led_by_credit_word(part) && std::ranges::any_of(known, [&](const std::vector<std::string>& name) { return starts_alike(part.subspan(1), name); });
}

bool is_credited_lead(std::span<const std::string> lead, const std::vector<std::vector<std::string>>& known) {
    // A composer before the work, by full name or surname, as Tchaikovsky in "Tchaikovsky: Swan Lake"
    return !lead.empty() && std::ranges::any_of(known, [&](const std::vector<std::string>& name) { return std::ranges::ends_with(name, lead); });
}

}
