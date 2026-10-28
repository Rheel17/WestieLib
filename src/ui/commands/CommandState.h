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
    bool log_visible = false;

    bool enabled(CommandId command) const;
    bool checked(CommandId command) const;
};

}
