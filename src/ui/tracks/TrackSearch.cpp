#include "ui/tracks/TrackSearch.h"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <string_view>

#include "platform/win32/Utf16.h"

namespace westie::ui {

namespace {

// Between the fields, so a word can't match across two of them
constexpr char FieldSeparator = '\n';
constexpr std::string_view WordSeparators = " \t";

}

std::string search_text(const platform::TextNormalizer& normalizer, const core::TrackMetadata& metadata) {
    std::string text = metadata.title;
    for (const std::string& artist : metadata.artists) {
        text += FieldSeparator;
        text += artist;
    }
    text += FieldSeparator;
    text += metadata.album;
    return normalizer.normalize(text);
}

std::vector<std::string> search_words(const platform::TextNormalizer& normalizer, std::wstring_view query) {
    std::string normalized = normalizer.normalize(platform::win32::to_utf8(query));
    std::vector<std::string> words;
    std::size_t start = normalized.find_first_not_of(WordSeparators);
    while (start != std::string::npos) {
        std::size_t end = normalized.find_first_of(WordSeparators, start);
        words.push_back(normalized.substr(start, end - start));
        start = normalized.find_first_not_of(WordSeparators, end);
    }
    return words;
}

std::vector<TrackRow> rows_matching(const std::vector<TrackRow>& rows, const std::vector<std::string>& words) {
    auto matches = [&](const TrackRow& row) {
        return std::ranges::all_of(words, [&](const std::string& word) { return row.search_text.contains(word); });
    };
    return rows | std::views::filter(matches) | std::ranges::to<std::vector>();
}

}
