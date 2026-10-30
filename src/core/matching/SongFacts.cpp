#include "core/matching/SongFacts.h"

#include <ranges>
#include <utility>

#include "core/matching/MatchWords.h"

namespace westie::core {

namespace {

constexpr char NameSeparator = ',';

}

SongFacts song_facts(
        const platform::TextNormalizer& normalizer, std::string_view title, const std::vector<std::string>& artists, std::string_view album,
        std::chrono::milliseconds duration, std::optional<bool> explicit_content
) {
    SongFacts song{
        .title = title_parts(normalizer, title),
        .album = album.empty() ? std::vector<std::string>() : whole_name(title_parts(normalizer, album)),
        .duration = duration,
        .explicit_content = explicit_content,
    };
    for (const std::string& artist : artists) {
        song.artists.push_back(match_words(normalizer, artist));
        if (!artist.contains(NameSeparator)) {
            continue;
        }
        for (auto piece : artist | std::views::split(NameSeparator)) {
            if (std::vector<std::string> name = match_words(normalizer, std::string_view(piece.begin(), piece.end())); !name.empty()) {
                song.artist_pieces.push_back(std::move(name));
            }
        }
    }
    return song;
}

}
