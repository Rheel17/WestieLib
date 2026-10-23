#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "core/sources/SourceType.h"
#include "core/tools/ToolPackage.h"
#include "ui/TrackCounts.h"

namespace westie::ui {

enum class PlaylistRun { None, Queued, Fetching };

struct PlaylistRow {
    std::string key{};
    core::SourceType type = core::SourceType::YouTubeMusic;
    std::wstring name{};
    std::wstring id{};
    std::wstring url{};
    TrackCounts counts{};
    // Tracks that other playlists of the same source list too
    int shared = 0;
    std::optional<std::chrono::system_clock::time_point> last_sync{};
    // When the last fetch failed; nothing after one that worked
    std::optional<std::chrono::system_clock::time_point> fetch_failed_at{};
    PlaylistRun run = PlaylistRun::None;
    // What its fetch waits for while it runs
    std::vector<core::ToolPackage> waiting_for{};
};

}
