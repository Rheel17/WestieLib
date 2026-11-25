#include "ui/commands/CommandState.h"

namespace westie::ui {

bool CommandState::enabled(CommandId command) const {
    switch (command) {
    case CommandId::RemovePlaylist:
    case CommandId::OpenPlaylistPage:
    case CommandId::CopyPlaylistLink:       return playlist_selected;
    // The welcome screen has no search
    case CommandId::FocusSearch:            return has_playlists;
    case CommandId::UpdateAll:              return has_playlists && !syncing;
    case CommandId::UpdateSelectedPlaylist: return playlist_selected && !syncing;
    case CommandId::CancelUpdate:           return syncing;
    case CommandId::CheckForUpdates:        return !tools_busy && !syncing;
    case CommandId::ConfirmMatch:           return review_selected;
    case CommandId::ChooseMatch:            return matched_entry_selected;
    // During an update too: the tracks join its queues
    case CommandId::Retry:                  return failed_selected;
    case CommandId::ShowInFolder:           return file_selected;
    case CommandId::Keep:                   return removed_selected;
    case CommandId::DeleteFiles:            return deletable_selected;
    case CommandId::EditTrack:
    case CommandId::OpenOnYouTube:
    case CommandId::OpenSourcePage:         return track_selected;
    default:                                return true;
    }
}

bool CommandState::checked(CommandId command) const {
    return command == CommandId::ToggleLog && log_visible;
}

}
