#include "ui/playlists/PlaylistMenu.h"

#include <array>

#include "platform/win32/UniqueHandle.h"
#include "ui/commands/CommandMenu.h"
#include "ui/framework/PopupMenu.h"

namespace westie::ui {

namespace {

constexpr std::array PlaylistItems{
    command_item(CommandId::SyncSelectedPlaylist, L"&Sync"),
    command_item(CommandId::OpenPlaylistPage, L"&Open in browser"),
    command_item(CommandId::CopyPlaylistLink, L"&Copy URL"),
    framework::MenuSeparator,
    command_item(CommandId::RemovePlaylist, L"&Remove playlist..."),
};

}

std::optional<CommandId> choose_playlist_command(HWND owner, POINT screen, const CommandState& state) {
    platform::win32::UniqueMenu menu = framework::build_popup_menu(PlaylistItems);
    update_menu_items(menu.get(), state);
    UINT flags = TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY | (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0 ? TPM_RIGHTALIGN : TPM_LEFTALIGN);
    // With TPM_RETURNCMD the result is the chosen command, or 0 when the menu closes without one
    auto chosen = static_cast<WORD>(TrackPopupMenu(menu.get(), flags, screen.x, screen.y, 0, owner, nullptr));
    if (chosen == 0) {
        return std::nullopt;
    }
    return static_cast<CommandId>(chosen);
}

}
