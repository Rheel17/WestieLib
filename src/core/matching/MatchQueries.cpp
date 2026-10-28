#include "core/matching/MatchQueries.h"

#include <format>
#include <string_view>
#include <utility>

#include "core/matching/MatchText.h"

namespace westie::core {

namespace {

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
