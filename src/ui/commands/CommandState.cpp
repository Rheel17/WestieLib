#include "ui/commands/CommandState.h"

namespace westie::ui {

bool CommandState::enabled(CommandId command) const {
    switch (command) {
    case CommandId::RemovePlaylist:
    case CommandId::OpenPlaylistPage:     return playlist_selected;
    case CommandId::SyncAll:              return has_playlists && !syncing;
    case CommandId::SyncSelectedPlaylist: return playlist_selected && !syncing;
    case CommandId::CancelSync:           return syncing;
    case CommandId::CheckForUpdates:      return !tools_busy && !syncing;
    case CommandId::ConfirmMatch:
    case CommandId::ChooseVideo:
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
