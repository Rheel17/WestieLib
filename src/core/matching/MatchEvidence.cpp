#include "core/matching/MatchEvidence.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "core/matching/Credits.h"
#include "core/matching/MatchWords.h"
#include "core/matching/NameMatch.h"

namespace westie::core {

namespace {

// The platforms round lengths to whole seconds, and some releases carry a little more silence
constexpr std::chrono::milliseconds SameLength{2000};
// Further apart than the same master usually is, but often still it
constexpr std::chrono::milliseconds CloseLength{5000};
// From here on it is another cut, as a radio edit or an extended mix
constexpr std::chrono::milliseconds OtherCutLength{10000};
// A name this long can be a letter off, as in a typo, and one twice as long two letters
constexpr std::size_t TypoLetters = 8;
constexpr std::size_t TwoTyposLetters = 16;
// Fewer words differ by one too easily, as Blues and Trixie Blues
constexpr std::size_t LeastWordsLeft = 2;
// Words that don't change what you hear, or whose change the length shows: the edition or master, the release or
// cut, the upload
constexpr std::array<std::string_view, 32> EditionWords = {
    "remaster", "remastered", "master", "reissue", "mono", "stereo", "digital", "digitally", "explicit", "version", "original", "album", "single", "edit",
    "radio", "extended", "mix", "short", "long", "full", "deluxe", "edition", "bonus", "track", "soundtrack", "official", "video", "audio", "lyric",
    "lyrics", "visualizer", "hd",
};
constexpr std::size_t YearDigits = 4;
constexpr int FirstYear = 1900;
constexpr int LastYear = 2099;
// Where a title as it is written breaks into words, and the marks around a word that aren't part of it
constexpr std::string_view WordBreaks = " ()[]{}";
constexpr std::array<std::string_view, 16> WordEdges = {"\"", "“", "”", "„", "«", "»", "「", "」", "『", "』", ",", ";", ":", "-", "–", "—"};

struct KnownNames {
    std::vector<std::vector<std::string>> for_wanted{};
    std::vector<std::vector<std::string>> for_found{};
};

bool is_edition_word(std::string_view word) {
    return std::ranges::contains(EditionWords, word);
}

bool is_number(std::string_view word) {
    return !word.empty() && std::ranges::all_of(word, [](char c) { return c >= '0' && c <= '9'; });
}

bool is_year(std::string_view word) {
    int year = 0;
    auto [end, error] = std::from_chars(word.data(), word.data() + word.size(), year);
    return word.size() == YearDigits && error == std::errc() && end == word.data() + word.size() && year >= FirstYear && year <= LastYear;
}

std::vector<std::vector<std::string>> joined(std::vector<std::vector<std::string>> names, const std::vector<std::vector<std::string>>& more) {
    names.insert(names.end(), more.begin(), more.end());
    return names;
}

std::vector<std::vector<std::string>> artist_names(const SongFacts& song) {
    // Each artist, and the names inside an entry that holds several, as The Unknown Blues Band in "Mike Griffin & The
    // Unknown Blues Band"
    std::vector<std::vector<std::string>> names = joined(song.artists, song.artist_pieces);
    for (const std::vector<std::string>& artist : song.artists) {
        names = joined(std::move(names), name_pieces(artist));
    }
    return names;
}

KnownNames known_names(const SongFacts& wanted, const SongFacts& found) {
    // Spotify's metadata is the reference. The found song's credits must name someone the wanted one credits, while the
    // wanted one's may name anyone either song credits, since YouTube Music often leaves guests out
    std::vector<std::vector<std::string>> wanted_artists = artist_names(wanted);
    return {
        .for_wanted = joined(joined(wanted_artists, artist_names(found)), credited_names(found.title)),
        .for_found = joined(wanted_artists, credited_names(wanted.title)),
    };
}

std::vector<std::string> song_name(const TitleParts& title, const std::vector<std::vector<std::string>>& known) {
    std::vector<std::string> name;
    if (!is_credited_lead(title.lead, known)) {
        name = title.lead;
    }
    name.insert(name.end(), title.name.begin(), title.name.end());
    if (!title.with_tail.empty() && !is_credit(title.with_tail, known)) {
        name.insert(name.end(), title.with_tail.begin(), title.with_tail.end());
    }
    return name;
}

std::vector<std::string> version_words(const TitleParts& title, const std::vector<std::vector<std::string>>& known) {
    std::vector<std::string> words;
    for (const std::vector<std::string>& part : title.parts) {
        if (is_credit(part, known)) {
            continue;
        }
        // A number beside such a word belongs to the same note, as in "Digitally Remastered 95" and "12" Version"
        bool edition = std::ranges::any_of(part, is_edition_word);
        for (const std::string& word : part) {
            if (!is_edition_word(word) && !(is_number(word) && (edition || is_year(word)))) {
                words.push_back(word);
            }
        }
    }
    return words;
}

bool same_version(std::vector<std::string> a, std::vector<std::string> b) {
    // Spacing aside too, as in StayLoose Remix and Stay Loose Remix
    if (letters_of(a) == letters_of(b)) {
        return true;
    }
    std::ranges::sort(a);
    a.erase(std::ranges::unique(a).begin(), a.end());
    std::ranges::sort(b);
    b.erase(std::ranges::unique(b).begin(), b.end());
    return a == b;
}

bool one_word_apart(std::span<const std::string> a, std::span<const std::string> b) {
    std::span<const std::string> shorter = a.size() < b.size() ? a : b;
    std::span<const std::string> longer = a.size() < b.size() ? b : a;
    if (longer.size() != shorter.size() + 1 || shorter.size() < LeastWordsLeft) {
        return false;
    }
    // The first word that differs is the one more
    auto [in_longer, in_shorter] = std::ranges::mismatch(longer, shorter);
    return std::ranges::equal(std::ranges::subrange(std::next(in_longer), longer.end()), std::ranges::subrange(in_shorter, shorter.end()));
}

std::size_t edit_distance(std::string_view a, std::string_view b) {
    std::vector<std::size_t> row(b.size() + 1);
    std::iota(row.begin(), row.end(), std::size_t{0});
    for (std::size_t i = 1; i <= a.size(); ++i) {
        std::size_t diagonal = row[0];
        row[0] = i;
        for (std::size_t j = 1; j <= b.size(); ++j) {
            std::size_t above = row[j];
            row[j] = std::min({row[j] + 1, row[j - 1] + 1, diagonal + (a[i - 1] == b[j - 1] ? std::size_t{0} : std::size_t{1})});
            diagonal = above;
        }
    }
    return row.back();
}

bool typos_apart(std::string_view a, std::string_view b) {
    std::size_t letters = std::min(a.size(), b.size());
    std::size_t allowed = letters >= TwoTyposLetters ? 2 : letters >= TypoLetters ? 1 : 0;
    return allowed > 0 && edit_distance(a, b) <= allowed;
}

NameEvidence name_evidence(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    std::string a_letters = letters_of(a);
    std::string b_letters = letters_of(b);
    if (a_letters == b_letters) {
        return NameEvidence::Same;
    }
    return one_word_apart(a, b) || typos_apart(a_letters, b_letters) ? NameEvidence::Close : NameEvidence::Other;
}

NameMatch closest_name(std::span<const std::string> name, const std::vector<std::vector<std::string>>& names) {
    NameMatch closest = NameMatch::Other;
    for (const std::vector<std::string>& other : names) {
        closest = std::min(closest, name_match(name, other));
    }
    return closest;
}

ArtistEvidence artist_evidence(const std::vector<std::vector<std::string>>& wanted, const std::vector<std::vector<std::string>>& found) {
    if (wanted.empty()) {
        return ArtistEvidence::Other;
    }
    switch (closest_name(wanted.front(), found)) {
    case NameMatch::Same:   return ArtistEvidence::Same;
    case NameMatch::Starts: return ArtistEvidence::Likely;
    case NameMatch::Inside: return ArtistEvidence::Doubtful;
    case NameMatch::Other:  break;
    }
    // Spotify now and then lists a remixer, a composer or a franchise first, so another of its artists still says a
    // little
    bool other = std::ranges::any_of(wanted | std::views::drop(1), [&](const std::vector<std::string>& name) {
        return closest_name(name, found) != NameMatch::Other;
    });
    return other ? ArtistEvidence::Doubtful : ArtistEvidence::Other;
}

LengthEvidence length_evidence(std::chrono::milliseconds wanted, std::chrono::milliseconds found) {
    if (wanted.count() <= 0 || found.count() <= 0) {
        return LengthEvidence::Unknown;
    }
    std::chrono::milliseconds off = std::chrono::abs(found - wanted);
    if (off <= SameLength) {
        return LengthEvidence::Same;
    }
    if (off <= CloseLength) {
        return LengthEvidence::Close;
    }
    return off < OtherCutLength ? LengthEvidence::Near : LengthEvidence::Off;
}

AlbumEvidence album_evidence(const std::vector<std::string>& wanted, const std::vector<std::string>& found) {
    if (wanted.empty() || found.empty()) {
        return AlbumEvidence::Unknown;
    }
    return letters_of(wanted) == letters_of(found) ? AlbumEvidence::Same : AlbumEvidence::Other;
}

ExplicitEvidence explicit_evidence(std::optional<bool> wanted, std::optional<bool> found) {
    if (!wanted || !found) {
        return ExplicitEvidence::Unknown;
    }
    return *wanted == *found ? ExplicitEvidence::Same : ExplicitEvidence::Other;
}

std::string_view without_edges(std::string_view word) {
    for (bool trimmed = true; trimmed && !word.empty();) {
        trimmed = false;
        for (std::string_view edge : WordEdges) {
            if (word.starts_with(edge)) {
                word.remove_prefix(edge.size());
                trimmed = true;
            }
            if (word.ends_with(edge)) {
                word.remove_suffix(edge.size());
                trimmed = true;
            }
        }
    }
    return word;
}

std::vector<std::string> written_words(const platform::TextNormalizer& normalizer, std::string_view title, const std::set<std::string>& words) {
    std::vector<std::string> written;
    std::set<std::string> seen;
    for (std::size_t start = 0; start < title.size();) {
        std::size_t end = std::min(title.find_first_of(WordBreaks, start), title.size());
        std::string_view word = without_edges(title.substr(start, end - start));
        std::vector<std::string> normalized = match_words(normalizer, word);
        bool named = std::ranges::any_of(normalized, [&](const std::string& part) { return words.contains(part); });
        // Once, though a title may say it twice, as "(Solo Acoustic) [Live] (Solo Acoustic; Live)"
        if (named && seen.insert(letters_of(normalized)).second) {
            written.emplace_back(word);
        }
        start = end + 1;
    }
    return written;
}

std::set<std::string> words_apart(const std::vector<std::string>& words, const std::vector<std::string>& others) {
    std::set<std::string> apart;
    for (const std::string& word : words) {
        if (!std::ranges::contains(others, word)) {
            apart.insert(word);
        }
    }
    return apart;
}

}

MatchEvidence match_evidence(const SongFacts& wanted, const SongFacts& found) {
    KnownNames known = known_names(wanted, found);
    return {
        .name = name_evidence(song_name(wanted.title, known.for_wanted), song_name(found.title, known.for_found)),
        // YouTube Music now and then lists several artists as one, as "Kina Grannis, Kurt Hugo Schneider"
        .artists = artist_evidence(wanted.artists, joined(found.artists, found.artist_pieces)),
        .same_version = same_version(version_words(wanted.title, known.for_wanted), version_words(found.title, known.for_found)),
        .length = length_evidence(wanted.duration, found.duration),
        .length_off = found.duration - wanted.duration,
        .album = album_evidence(wanted.album, found.album),
        .explicit_content = explicit_evidence(wanted.explicit_content, found.explicit_content),
    };
}

VersionDifference version_difference(
        const platform::TextNormalizer& normalizer, std::string_view wanted_title, const std::vector<std::string>& wanted_artists, std::string_view found_title,
        const std::vector<std::string>& found_artists
) {
    SongFacts wanted = song_facts(normalizer, wanted_title, wanted_artists, "", {});
    SongFacts found = song_facts(normalizer, found_title, found_artists, "", {});
    KnownNames known = known_names(wanted, found);
    std::vector<std::string> wanted_words = version_words(wanted.title, known.for_wanted);
    std::vector<std::string> found_words = version_words(found.title, known.for_found);
    return {
        .wanted = written_words(normalizer, wanted_title, words_apart(wanted_words, found_words)),
        .found = written_words(normalizer, found_title, words_apart(found_words, wanted_words)),
    };
}

}
