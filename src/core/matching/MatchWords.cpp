#include "core/matching/MatchWords.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

namespace westie::core {

namespace {

struct Replacement {
    std::string_view from{};
    std::string_view to{};
};

struct Abbreviation {
    std::string_view word{};
    std::string_view meaning{};
};

// Marks that NFKD leaves and that don't tell songs apart, quotes as one mark, which title parts read, and symbols as
// they are read: & and + as in Dan + Shay, × as in A × B, $ as in Ke$ha
constexpr std::array Replacements = {
    Replacement{"’", ""}, Replacement{"‘", ""}, Replacement{"'", ""}, Replacement{"`", ""}, Replacement{"“", "\""}, Replacement{"”", "\""},
    Replacement{"„", "\""}, Replacement{"«", "\""}, Replacement{"»", "\""}, Replacement{"「", "\""}, Replacement{"」", "\""}, Replacement{"『", "\""},
    Replacement{"』", "\""}, Replacement{"・", " "}, Replacement{"、", " "}, Replacement{"。", " "}, Replacement{"–", "-"}, Replacement{"—", "-"},
    Replacement{"‐", "-"}, Replacement{"&", " and "}, Replacement{"+", " and "}, Replacement{"×", " x "}, Replacement{"$", "s"},
};
constexpr std::array Abbreviations = {
    Abbreviation{"pt", "part"}, Abbreviation{"vol", "volume"}, Abbreviation{"ver", "version"}, Abbreviation{"ft", "feat"}, Abbreviation{"featuring", "feat"},
    Abbreviation{"versus", "vs"},
};

bool is_word_byte(char c) {
    // Bytes from 0x80 are parts of letters outside ASCII, which stay
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || static_cast<unsigned char>(c) >= 0x80;
}

std::string replaced(std::string text) {
    for (const Replacement& replacement : Replacements) {
        for (std::size_t at = text.find(replacement.from); at != std::string::npos; at = text.find(replacement.from, at + replacement.to.size())) {
            text.replace(at, replacement.from.size(), replacement.to);
        }
    }
    return text;
}

std::string joined_hyphens(std::string_view text) {
    std::string joined;
    for (std::size_t at = 0; at < text.size(); ++at) {
        // A hyphen inside a word is part of it, as in re-recorded and JAY-Z; one with a space beside it separates
        bool inside_word = at > 0 && at + 1 < text.size() && is_word_byte(text[at - 1]) && is_word_byte(text[at + 1]);
        if (text[at] != '-' || !inside_word) {
            joined += text[at];
        }
    }
    return joined;
}

std::string written_out(std::string word) {
    auto abbreviation = std::ranges::find(Abbreviations, word, &Abbreviation::word);
    return abbreviation == Abbreviations.end() ? std::move(word) : std::string(abbreviation->meaning);
}

}

std::string folded_text(const platform::TextNormalizer& normalizer, std::string_view text) {
    return joined_hyphens(replaced(normalizer.normalize(text)));
}

std::vector<std::string> words_of(std::string_view folded) {
    std::vector<std::string> words;
    std::string word;
    for (char c : folded) {
        if (is_word_byte(c)) {
            word += c;
        } else if (!word.empty()) {
            words.push_back(written_out(std::exchange(word, {})));
        }
    }
    if (!word.empty()) {
        words.push_back(written_out(std::move(word)));
    }
    return words;
}

std::vector<std::string> match_words(const platform::TextNormalizer& normalizer, std::string_view text) {
    return words_of(folded_text(normalizer, text));
}

std::string letters_of(std::span<const std::string> words) {
    std::string letters;
    for (const std::string& word : words) {
        letters += word;
    }
    return letters;
}

}
