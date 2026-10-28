#include "ui/commands/CommandState.h"

namespace westie::ui {

bool CommandState::enabled(CommandId command) const {
    switch (command) {
    case CommandId::RemovePlaylist:
    case CommandId::OpenPlaylistPage:
    case CommandId::CopyPlaylistLink:     return playlist_selected;
    // The welcome screen has no search
    case CommandId::FocusSearch:          return has_playlists;
    case CommandId::SyncAll:              return has_playlists && !syncing;
    case CommandId::SyncSelectedPlaylist: return playlist_selected && !syncing;
    case CommandId::CancelSync:           return syncing;
    case CommandId::CheckForUpdates:      return !tools_busy && !syncing;
    case CommandId::ConfirmMatch:         return review_selected;
    case CommandId::ChooseVideo:          return spotify_entry_selected;
    case CommandId::EditTrack:
    case CommandId::Retry:
    case CommandId::Keep:
    case CommandId::MoveToRecycleBin:
    case CommandId::ShowInFolder:
    case CommandId::OpenOnYouTube:
    case CommandId::OpenSourcePage:       return track_selected;
    default:                              return true;
    }
}

bool CommandState::checked(CommandId command) const {
    return command == CommandId::ToggleLog && log_visible;
}

}
