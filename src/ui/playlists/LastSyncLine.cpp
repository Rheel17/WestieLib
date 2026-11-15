#include "ui/playlists/LastSyncLine.h"

#include <algorithm>
#include <format>

#include "ui/RelativeDayText.h"
#include "ui/sync/WaitingText.h"

namespace westie::ui {

LastSyncLine last_sync_line(const PlaylistRow& row, std::chrono::system_clock::time_point now) {
    switch (row.run) {
    case PlaylistRun::Syncing:
        if (!row.waiting_for.empty()) {
            return {.mark = SyncMark::WaitingForTools, .text = waiting_for_text(row.waiting_for), .control = SyncControl::Queued};
        }
        return {.text = L"Syncing…", .control = SyncControl::Spinning};
    case PlaylistRun::Matching:
        // As the status bar does, the track being matched counts
        return {
            .text = std::format(L"Matching {} of {}", row.matching_now ? std::min(row.matched + 1, row.to_match) : row.matched, row.to_match),
            .control = row.matching_now ? SyncControl::Spinning : SyncControl::Queued,
        };
    case PlaylistRun::Queued: return {.text = L"Queued", .control = SyncControl::Queued};
    case PlaylistRun::None:   break;
    }

    if (row.fetch_failed_at) {
        std::wstring updated = row.last_sync ? L", updated " + relative_day_text(*row.last_sync, now) : L"";
        return {.mark = SyncMark::FetchFailed, .warning = L"Couldn't sync", .text = std::move(updated)};
    }
    std::wstring text = row.last_sync ? L"Updated " + relative_day_text(*row.last_sync, now) : L"Never updated";
    SyncMark mark = SyncMark::Synced;
    if (row.counts.failed > 0) {
        mark = SyncMark::Failures;
    } else if (row.counts.waiting > 0 || !row.last_sync) {
        mark = SyncMark::Unsynced;
    }
    return {.mark = mark, .text = std::move(text)};
}

}
