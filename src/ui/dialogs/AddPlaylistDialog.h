#pragma once

#include <functional>
#include <optional>

#include <windows.h>

#include "core/playlists/PlaylistLink.h"
#include "ui/dialogs/PlaylistLinkCheck.h"
#include "ui/framework/ModalDialog.h"

namespace westie::ui {

struct AddPlaylistRequest {
    core::PlaylistLink link{};
    bool sync_now = true;
};

class AddPlaylistDialog final : public framework::ModalDialog {
public:
    explicit AddPlaylistDialog(std::function<bool(const core::PlaylistLink&)> is_added);

    std::optional<AddPlaylistRequest> choose(HWND owner);

protected:
    INT_PTR handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void on_init_dialog();
    void on_command(WPARAM wparam);
    void check_link();
    void draw_link_line(const DRAWITEMSTRUCT& item) const;
    void accept();

    std::function<bool(const core::PlaylistLink&)> is_added_;
    PlaylistLinkCheck check_;
    std::optional<AddPlaylistRequest> request_;
};

}
