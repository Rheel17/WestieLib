#pragma once

#include <chrono>
#include <string>

#include "ui/playlists/PlaylistRow.h"

namespace westie::ui {

// The status icon that starts the line; a run supersedes it
enum class SyncMark { None, FetchFailed, Failures, WaitingForTools, Unsynced, Synced };

enum class SyncControl { Idle, Spinning, Queued };

struct LastSyncLine {
    SyncMark mark = SyncMark::None;
    // In amber, before the text
    std::wstring warning{};
    std::wstring text{};
    SyncControl control = SyncControl::Idle;
};

LastSyncLine last_sync_line(const PlaylistRow& row, std::chrono::system_clock::time_point now);

}
