#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include "core/matching/SongFacts.h"
#include "platform/TextNormalizer.h"

namespace westie::core {

// Each ordered from the closest
enum class NameEvidence { Same, Close, Other };
enum class ArtistEvidence { Same, Likely, Doubtful, Other };
enum class LengthEvidence { Same, Close, Near, Unknown, Off };
enum class AlbumEvidence { Same, Unknown, Other };
enum class ExplicitEvidence { Same, Unknown, Other };

struct MatchEvidence {
    NameEvidence name = NameEvidence::Other;
    ArtistEvidence artists = ArtistEvidence::Other;
    bool same_version = false;
    LengthEvidence length = LengthEvidence::Unknown;
    // The found song's length minus the wanted one's
    std::chrono::milliseconds length_off{};
    AlbumEvidence album = AlbumEvidence::Unknown;
    ExplicitEvidence explicit_content = ExplicitEvidence::Unknown;
};

struct VersionDifference {
    // The version words only that title has, as it writes them and in its order: "Korean" for "Love U Like That -
    // Korean Version" against "Love U Like That"
    std::vector<std::string> wanted{};
    std::vector<std::string> found{};
};

MatchEvidence match_evidence(const SongFacts& wanted, const SongFacts& found);
VersionDifference version_difference(
        const platform::TextNormalizer& normalizer, std::string_view wanted_title, const std::vector<std::string>& wanted_artists, std::string_view found_title,
        const std::vector<std::string>& found_artists
);

}
