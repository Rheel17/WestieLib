#include "core/matching/MatchQueries.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <ranges>
#include <string_view>
#include <utility>

namespace westie::core {

namespace {

constexpr std::array<std::string_view, 3> SuffixSeparators = {" - ", " – ", " — "};
constexpr std::array<std::string_view, 5> FeaturingWords = {" feat. ", " feat ", " ft. ", " ft ", " featuring "};

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

std::string bare_title(std::string_view title) {
    // As one would type it: without brackets, a " - " suffix or a feat. tail
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

std::string joined(const std::vector<std::string>& parts, std::string_view separator) {
    std::string text;
    for (const std::string& part : parts) {
        if (!text.empty()) {
            text += separator;
        }
        text += part;
    }
    return text;
}

std::string query_of(std::string_view artists, std::string_view title) {
    return artists.empty() ? std::string(title) : std::format("{} {}", artists, title);
}

}

std::vector<std::string> match_queries(const Source& source) {
    std::vector<std::string> queries = {query_of(joined(source.artists, " "), source.title)};
    std::string bare = query_of(source.artists.empty() ? std::string() : source.artists.front(), bare_title(source.title));
    if (bare != queries.front()) {
        queries.push_back(std::move(bare));
    }
    return queries;
}

}
