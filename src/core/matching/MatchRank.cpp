#include "core/matching/MatchRank.h"

#include <chrono>
#include <utility>

namespace westie::core {

MatchRank match_rank(const MatchJudgement& judgement, bool known_ytm_id) {
    const MatchEvidence& evidence = judgement.evidence;
    // The verdict enums run from the worst, the evidence enums from the closest
    return {
        .verdict = std::to_underlying(judgement.verdict),
        .mismatch = std::to_underlying(judgement.mismatch),
        .fewer_doubts = -static_cast<int>(judgement.doubts.size()),
        .artists = std::to_underlying(ArtistEvidence::Other) - std::to_underlying(evidence.artists),
        .length = std::to_underlying(LengthEvidence::Off) - std::to_underlying(evidence.length),
        .known_ytm_id = known_ytm_id,
        .same_album = evidence.album == AlbumEvidence::Same,
        .explicit_content = std::to_underlying(ExplicitEvidence::Other) - std::to_underlying(evidence.explicit_content),
        .name = std::to_underlying(NameEvidence::Other) - std::to_underlying(evidence.name),
        .closeness = -std::chrono::abs(evidence.length_off).count(),
    };
}

}
