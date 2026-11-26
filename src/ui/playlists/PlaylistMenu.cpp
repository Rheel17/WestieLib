#include "ui/playlists/PlaylistMenu.h"

#include <array>

#include "ui/commands/CommandMenu.h"
#include "ui/framework/PopupMenu.h"

namespace westie::ui {

namespace {

constexpr std::array PlaylistItems{
    command_item(CommandId::UpdateSelectedPlaylist, L"&Update"),         command_item(CommandId::OpenPlaylistPage, L"&Open in browser"),
    command_item(CommandId::CopyPlaylistLink, L"&Copy link"),        framework::MenuSeparator,
    command_item(CommandId::RemovePlaylist, L"&Remove playlist…\tDel"),
};

}

std::optional<CommandId> choose_playlist_command(HWND owner, POINT screen, const CommandState& state) {
    return choose_command(owner, screen, PlaylistItems, state);
}

}
