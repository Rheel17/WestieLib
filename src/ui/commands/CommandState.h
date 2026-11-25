#pragma once

#include "ui/commands/CommandId.h"

namespace westie::ui {

struct CommandState {
    bool syncing = false;
    bool tools_busy = false;
    bool has_playlists = false;
    bool playlist_selected = false;
    bool track_selected = false;
    // Exactly one row, and it needs review
    bool review_selected = false;
    // Exactly one row, and its entry is a Spotify track
    bool spotify_entry_selected = false;
    // At least one row whose track failed
    bool failed_selected = false;
    // At least one row whose track is Removed, and one whose track is Removed or Kept
    bool removed_selected = false;
    bool deletable_selected = false;
    // Exactly one row, and its track has a file
    bool file_selected = false;
    bool log_visible = false;

    bool enabled(CommandId command) const;
    bool checked(CommandId command) const;
};

}
