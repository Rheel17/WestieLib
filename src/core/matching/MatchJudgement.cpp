#include "core/matching/MatchJudgement.h"

#include <algorithm>

#include "core/state/MatchReview.h"

namespace westie::core {

namespace {

constexpr double AcceptConfidence = 1.0;
// Accepted on a length or artists that are close rather than the same
constexpr double LooserAcceptConfidence = 0.9;
constexpr double ReviewConfidence = 0.75;
// Less for each doubt after the first
constexpr double DoubtStep = 0.1;
constexpr double LeastReviewConfidence = 0.55;
constexpr double OtherRecordingConfidence = 0.3;
constexpr double OtherArtistConfidence = 0.15;
constexpr double OtherSongConfidence = 0.05;
static_assert(LooserAcceptConfidence >= ReviewThreshold && ReviewConfidence < ReviewThreshold);

double rejected_confidence(MatchMismatch mismatch) {
    switch (mismatch) {
    case MatchMismatch::Recording: return OtherRecordingConfidence;
    case MatchMismatch::Artist:    return OtherArtistConfidence;
    case MatchMismatch::Song:
    case MatchMismatch::None:      break;
    }
    return OtherSongConfidence;
}

MatchJudgement rejected(const MatchEvidence& evidence, MatchMismatch mismatch) {
    return {.verdict = MatchVerdict::Reject, .mismatch = mismatch, .evidence = evidence, .confidence = rejected_confidence(mismatch)};
}

bool length_agrees(const MatchEvidence& evidence) {
    // Further off, only the same album says it is still the same cut
    return evidence.length == LengthEvidence::Same || evidence.length == LengthEvidence::Close ||
           (evidence.length == LengthEvidence::Near && evidence.album == AlbumEvidence::Same);
}

bool doubts_artist(const MatchEvidence& evidence, bool alias) {
    bool same_length = evidence.length == LengthEvidence::Same;
    bool same_album = evidence.album == AlbumEvidence::Same;
    switch (evidence.artists) {
    case ArtistEvidence::Same:     return false;
    case ArtistEvidence::Likely:   return !same_length && !same_album;
    case ArtistEvidence::Doubtful: return !same_length || !same_album;
    case ArtistEvidence::Other:    return alias;
    }
    return true;
}

}

MatchJudgement judge_match(const SongFacts& wanted, const SongFacts& found) {
    MatchEvidence evidence = match_evidence(wanted, found);
    bool same_length = evidence.length == LengthEvidence::Same;
    if (evidence.name == NameEvidence::Other) {
        return rejected(evidence, MatchMismatch::Song);
    }
    // Another name for the same artist, as Mama Cass for Cass Elliot, shows only when everything else agrees
    bool alias = evidence.name == NameEvidence::Same && evidence.same_version && same_length && evidence.album == AlbumEvidence::Same;
    if (evidence.artists == ArtistEvidence::Other && !alias) {
        return rejected(evidence, MatchMismatch::Artist);
    }
    // A live version or another remix has a length of its own
    if (!evidence.same_version && !same_length) {
        return rejected(evidence, MatchMismatch::Recording);
    }

    MatchJudgement judgement{.mismatch = MatchMismatch::None, .evidence = evidence};
    if (evidence.name == NameEvidence::Close) {
        judgement.doubts.push_back(MatchDoubt::Title);
    }
    if (doubts_artist(evidence, alias)) {
        judgement.doubts.push_back(MatchDoubt::Artist);
    }
    if (!evidence.same_version) {
        judgement.doubts.push_back(MatchDoubt::Version);
    }
    if (!length_agrees(evidence)) {
        judgement.doubts.push_back(MatchDoubt::Length);
    }
    if (evidence.explicit_content == ExplicitEvidence::Other) {
        judgement.doubts.push_back(MatchDoubt::Explicit);
    }

    if (judgement.doubts.empty()) {
        judgement.verdict = MatchVerdict::Accept;
        judgement.confidence = same_length && evidence.artists == ArtistEvidence::Same ? AcceptConfidence : LooserAcceptConfidence;
    } else {
        judgement.verdict = MatchVerdict::Review;
        double more_doubts = static_cast<double>(judgement.doubts.size() - 1);
        judgement.confidence = std::max(LeastReviewConfidence, ReviewConfidence - DoubtStep * more_doubts);
    }
    return judgement;
}

}
