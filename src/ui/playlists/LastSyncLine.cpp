#include "ui/playlists/LastSyncLine.h"

#include "ui/RelativeDayText.h"
#include "ui/sync/WaitingText.h"

namespace westie::ui {

LastSyncLine last_sync_line(const PlaylistRow& row, std::chrono::system_clock::time_point now) {
    switch (row.run) {
    case PlaylistRun::Fetching:
        if (!row.waiting_for.empty()) {
            return {.mark = SyncMark::WaitingForTools, .text = waiting_for_text(row.waiting_for), .control = SyncControl::Queued};
        }
        return {.text = L"Syncing…", .control = SyncControl::Spinning};
    case PlaylistRun::Queued: return {.text = L"Queued", .control = SyncControl::Queued};
    case PlaylistRun::None:   break;
    }

    if (row.fetch_failed_at) {
        std::wstring synced = row.last_sync ? L" · synced " + relative_day_text(*row.last_sync, now) : L"";
        return {.mark = SyncMark::FetchFailed, .warning = L"Couldn't fetch", .text = std::move(synced)};
    }
    std::wstring text = row.last_sync ? L"Synced " + relative_day_text(*row.last_sync, now) : L"Never synced";
    SyncMark mark = SyncMark::Synced;
    if (row.counts.failed > 0) {
        mark = SyncMark::Failures;
    } else if (row.counts.waiting > 0 || !row.last_sync) {
        mark = SyncMark::Unsynced;
    }
    return {.mark = mark, .text = std::move(text)};
}

}
