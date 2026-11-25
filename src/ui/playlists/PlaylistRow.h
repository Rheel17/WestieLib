#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "core/sources/SourceType.h"
#include "core/tools/ToolPackage.h"
#include "ui/TrackCounts.h"

namespace westie::ui {

enum class PlaylistRun { None, Queued, Syncing, Matching, Downloading };

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
    // What its sync, or its downloads, wait for while it runs
    std::vector<core::ToolPackage> waiting_for{};
    // While it matches: its tracks the run queued to match, how many of them ended, and whether one is matched now
    int matched = 0;
    int to_match = 0;
    bool matching_now = false;
    // While it downloads: its tracks the run queued to download, how many of them ended, and how many download now
    int downloaded = 0;
    int to_download = 0;
    int downloading_now = 0;
};

}
