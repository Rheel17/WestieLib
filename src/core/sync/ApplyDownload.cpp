#include "core/sync/ApplyDownload.h"

#include <algorithm>
#include <ranges>
#include <utility>

#include "core/sources/SourceType.h"

namespace westie::core {

bool apply_download(State& state, const std::string& track_key, const std::string& ytm_id, const std::filesystem::path& file, const VideoInfo& info) {
    auto track = state.tracks.find(track_key);
    // Gone with its playlist while it downloaded
    if (track == state.tracks.end()) {
        return false;
    }
    track->second.status = TrackStatus::Downloaded;
    track->second.ytm_id = ytm_id;
    track->second.file = file;
    track->second.failure = TrackFailure::None;
    track->second.error.clear();
    // A fetch never tells these; the video's info does
    for (const std::string& key : track->second.sources) {
        auto source = state.sources.find(key);
        if (source == state.sources.end() || source_type_of_key(key) != SourceType::YouTubeMusic) {
            continue;
        }
        if (!info.album.empty()) {
            source->second.album = info.album;
        }
        if (info.year) {
            source->second.year = info.year;
        }
    }
    return true;
}

void apply_download_failure(State& state, const std::string& track_key, std::string error) {
    auto track = state.tracks.find(track_key);
    if (track == state.tracks.end()) {
        return;
    }
    track->second.status = TrackStatus::Failed;
    track->second.failure = TrackFailure::Download;
    track->second.error = std::move(error);
}

// False for the track's override: the user chose that id, so it fails as any download does
bool apply_unavailable(State& state, const std::string& track_key, const std::string& ytm_id) {
    auto track = state.tracks.find(track_key);
    if (track == state.tracks.end() || track->second.ytm_override == ytm_id) {
        return false;
    }
    state.unavailable_ytm_ids.insert(ytm_id);
    // Every match to it, so no other track that waits for review downloads it either
    for (Source& source : state.sources | std::views::values) {
        if (source.match && source.match->ytm_id == ytm_id) {
            source.match.reset();
        }
    }
    track->second.status = TrackStatus::Pending;
    track->second.failure = TrackFailure::None;
    track->second.error.clear();
    return true;
}

// The failed ones among them wait again, and are the ones it returns
std::vector<std::string> apply_retry(State& state, const std::vector<std::string>& track_keys) {
    std::vector<std::string> retried;
    for (const std::string& key : track_keys) {
        auto track = state.tracks.find(key);
        if (track == state.tracks.end() || track->second.status != TrackStatus::Failed || std::ranges::contains(retried, key)) {
            continue;
        }
        track->second.status = TrackStatus::Pending;
        track->second.failure = TrackFailure::None;
        track->second.error.clear();
        retried.push_back(key);
    }
    return retried;
}

}
