#pragma once

#include <functional>

#include <windows.h>

#include "ui/commands/CommandId.h"
#include "ui/commands/CommandState.h"
#include "ui/framework/Button.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Label.h"
#include "ui/framework/LinkLabel.h"
#include "ui/framework/Panel.h"
#include "ui/inspector/CardTitle.h"
#include "ui/inspector/SectionTitle.h"
#include "ui/inspector/SyncedValue.h"
#include "ui/playlists/PlaylistRow.h"

namespace westie::ui {

class PlaylistCard final : public framework::Panel {
public:
    PlaylistCard(HWND parent, std::function<void(CommandId)> on_command);

    int height_for(int width) const;
    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void set_contents(const PlaylistRow& playlist);
    void update(const CommandState& state);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    int arrange(int width, framework::DeferredPositions* positions) const;
    void layout();
    void on_command(WPARAM wparam) const;

    std::function<void(CommandId)> on_command_;
    CardTitle title_;
    framework::Label subtitle_;
    SectionTitle section_;
    framework::Label link_key_;
    framework::LinkLabel link_;
    framework::Label tracks_key_;
    framework::Label tracks_;
    framework::Label synced_key_;
    SyncedValue synced_;
    framework::Label last_sync_key_;
    framework::Label last_sync_;
    framework::Button sync_;
    framework::Button remove_;
    int line_height_ = 0;
};

}
