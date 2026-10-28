#include "ui/dialogs/VideoChoice.h"

#include <cmath>
#include <format>
#include <utility>

#include "ResourceIds.h"
#include "core/matching/MatchQueries.h"
#include "core/sources/SourceType.h"
#include "core/state/TrackVideos.h"
#include "platform/win32/Utf16.h"
#include "ui/ArtistsText.h"
#include "ui/DurationText.h"

namespace westie::ui {

namespace {

constexpr double PercentPerWhole = 100.0;

std::wstring heading_details(const core::Source& source) {
    std::wstring details;
    for (const std::wstring& part : {artists_text(source.artists), platform::win32::to_utf16(source.album), duration_text(source.duration)}) {
        if (!part.empty()) {
            details += details.empty() ? L"· " : L" · ";
            details += part;
        }
    }
    return details;
}

std::wstring current_text(const core::State& state, const core::Track& track) {
    std::optional<core::ResolvedVideo> video = core::resolved_video(state, track);
    if (!video) {
        bool failed = track.status == core::TrackStatus::Failed && track.failure == core::TrackFailure::NoMatch;
        return failed ? L"The Matcher found no usable video." : L"The Matcher hasn't looked for a video yet.";
    }
    if (video->overridden) {
        std::string title = library_video_title(state, video->video_id).value_or(video->video_id);
        return std::format(L"Current video: {}, chosen by you", platform::win32::to_utf16(title));
    }
    auto source = state.sources.find(video->source);
    if (source == state.sources.end()) {
        return L"";
    }
    if (const std::optional<core::Match>& match = source->second.match; match && !core::own_video(video->source)) {
        return std::format(L"Current match: {}, {}% confidence", platform::win32::to_utf16(match->title), std::lround(match->confidence * PercentPerWhole));
    }
    return std::format(L"Current video: {}, from a YouTube Music playlist", platform::win32::to_utf16(source->second.title));
}

}

std::optional<VideoChoice> video_choice(const core::State& state, const std::string& source_key) {
    auto source = state.sources.find(source_key);
    if (source == state.sources.end() || core::source_type_of_key(source_key) != core::SourceType::Spotify) {
        return std::nullopt;
    }
    auto track = state.tracks.find(source->second.track);
    if (track == state.tracks.end()) {
        return std::nullopt;
    }
    LinkLine heading{.icon = IDI_SPOTIFY, .text = platform::win32::to_utf16(source->second.title), .source = heading_details(source->second)};
    return VideoChoice{
        .source = source_key,
        .heading = std::move(heading),
        .current = current_text(state, track->second),
        .query = platform::win32::to_utf16(core::match_queries(source->second).front()),
        .duration = source->second.duration,
    };
}

std::optional<std::string> library_video_title(const core::State& state, const std::string& video_id) {
    auto source = state.sources.find(std::format("{}:{}", core::source_prefix(core::SourceType::YouTubeMusic), video_id));
    if (source == state.sources.end()) {
        return std::nullopt;
    }
    return source->second.title;
}

}
