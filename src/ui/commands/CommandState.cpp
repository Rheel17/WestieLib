#include "ui/commands/CommandState.h"

namespace westie::ui {

bool CommandState::enabled(CommandId command) const {
    bool can_sync = tools_ready && !tools_busy && !syncing;
    switch (command) {
    case CommandId::RemovePlaylist:       return playlist_selected;
    case CommandId::SyncAll:              return can_sync;
    case CommandId::SyncSelectedPlaylist: return can_sync && playlist_selected;
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
