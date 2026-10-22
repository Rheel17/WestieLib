#pragma once

#include <optional>
#include <string>

#include <windows.h>

#include "core/playlists/PlaylistLink.h"
#include "core/playlists/PlaylistLookedUp.h"
#include "core/playlists/PlaylistLookup.h"
#include "ui/dialogs/LinkLookup.h"
#include "ui/dialogs/PlaylistLinkCheck.h"
#include "ui/framework/Animation.h"
#include "ui/framework/ModalDialog.h"
#include "ui/tools/ToolBoard.h"

namespace westie::ui {

struct AddPlaylistRequest {
    core::PlaylistLink link{};
    std::string name{};
    bool sync_now = true;
};

class AddPlaylistDialog final : public framework::ModalDialog {
public:
    AddPlaylistDialog(AddedPlaylistName added_name, core::PlaylistLookup& lookup, const ToolBoard& tools);

    std::optional<AddPlaylistRequest> choose(HWND owner);
    void show_lookup(const core::PlaylistLookedUp& answer);
    void show_tools();

protected:
    INT_PTR handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void on_init_dialog();
    void on_command(WPARAM wparam);
    void on_timer(WPARAM timer);
    void check_link();
    void show_line();
    bool can_add() const noexcept;
    void accept();

    AddedPlaylistName added_name_;
    LinkLookup lookup_;
    framework::Animation spinner_;
    PlaylistLinkCheck check_;
    std::optional<AddPlaylistRequest> request_;
};

}
