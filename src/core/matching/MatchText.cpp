#include "core/matching/MatchText.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <utility>

namespace westie::core {

namespace {

struct Replacement {
    std::string_view from{};
    std::string_view to{};
};

// Typographic marks that NFKD leaves, and & as it is spoken
constexpr std::array Replacements = {
    Replacement{"’", ""}, Replacement{"‘", ""}, Replacement{"'", ""}, Replacement{"“", " "}, Replacement{"”", " "},
    Replacement{"–", "-"}, Replacement{"—", "-"}, Replacement{"&", " and "},
};
constexpr std::array<std::string_view, 3> SuffixSeparators = {" - ", " – ", " — "};
constexpr std::array<std::string_view, 5> FeaturingWords = {" feat. ", " feat ", " ft. ", " ft ", " featuring "};
// Each a different recording than the one without it
constexpr std::array<std::string_view, 20> VersionWords = {
    "live", "remix", "cover", "karaoke", "sped up", "slowed", "instrumental", "acoustic", "stripped", "session", "sessions", "recorded at", "piano",
    "orchestral", "demo", "nightcore", "reverb", "in the style of", "made famous by", "tribute",
};

std::string replaced(std::string text) {
    for (const Replacement& replacement : Replacements) {
        for (std::size_t at = text.find(replacement.from); at != std::string::npos; at = text.find(replacement.from, at + replacement.to.size())) {
            text.replace(at, replacement.from.size(), replacement.to);
        }
    }
    return text;
}

std::string without_brackets(std::string_view text) {
    std::string result;
    int depth = 0;
    for (char c : text) {
        if (c == '(' || c == '[') {
            ++depth;
            // A space keeps the words on either side apart
            result += ' ';
        } else if ((c == ')' || c == ']') && depth > 0) {
            --depth;
        } else if (depth == 0) {
            result += c;
        }
    }
    return result;
}

char ascii_lower(char c) {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

std::string_view trimmed(std::string_view text) {
    while (text.starts_with(' ')) {
        text.remove_prefix(1);
    }
    while (text.ends_with(' ')) {
        text.remove_suffix(1);
    }
    return text;
}

bool is_word_byte(char c) {
    // Bytes from 0x80 are parts of letters outside ASCII, which stay
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || static_cast<unsigned char>(c) >= 0x80;
}

std::vector<std::string> words_of(std::string_view text) {
    std::vector<std::string> words;
    std::string word;
    for (char c : text) {
        if (is_word_byte(c)) {
            word += c;
        } else if (!word.empty()) {
            words.push_back(std::exchange(word, {}));
        }
    }
    if (!word.empty()) {
        words.push_back(std::move(word));
    }
    return words;
}

bool contains_words(const std::vector<std::string>& words, std::string_view phrase) {
    std::vector<std::string> wanted = words_of(phrase);
    return !std::ranges::search(words, wanted).empty();
}

}

std::string bare_title(std::string_view title) {
    std::string bare = without_brackets(title);
    for (std::string_view separator : SuffixSeparators) {
        if (std::size_t at = bare.find(separator); at != std::string::npos) {
            bare.resize(at);
        }
    }
    // Padded, so a feat. at either end is a word too
    std::string padded = " " + (bare | std::views::transform(ascii_lower) | std::ranges::to<std::string>()) + " ";
    for (std::string_view featuring : FeaturingWords) {
        if (std::size_t at = padded.find(featuring); at != std::string::npos) {
            bare.resize(std::min(bare.size(), at == 0 ? std::size_t{0} : at - 1));
            padded.resize(at + 1);
        }
    }
    std::string_view result = trimmed(bare);
    return std::string(result.empty() ? trimmed(title) : result);
}

MatchText match_text(const platform::TextNormalizer& normalizer, std::string_view text) {
    std::string normalized = replaced(normalizer.normalize(text));
    std::vector<std::string> all = words_of(normalized);
    MatchText match{.words = words_of(bare_title(normalized))};
    if (match.words.empty()) {
        match.words = all;
    }
    for (std::string_view version : VersionWords) {
        if (contains_words(all, version)) {
            match.versions.emplace_back(version);
        }
    }
    std::ranges::sort(match.versions);
    return match;
}

}
