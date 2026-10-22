#pragma once

#include <filesystem>
#include <functional>

#include <windows.h>

#include "ui/TrackCounts.h"
#include "ui/commands/CommandId.h"
#include "ui/commands/CommandState.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Label.h"
#include "ui/framework/Panel.h"
#include "ui/framework/VerticalScroll.h"
#include "ui/inspector/LibraryCard.h"
#include "ui/inspector/PlaylistCard.h"
#include "ui/playlists/PlaylistRow.h"

namespace westie::ui {

class InspectorPanel final : public framework::Panel {
public:
    InspectorPanel(HWND parent, const std::function<void(CommandId)>& on_command);

    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void show_first_run();
    void show_library(const std::filesystem::path& folder, const TrackCounts& counts);
    void show_playlist(const PlaylistRow& playlist);
    void update(const CommandState& state);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    enum class Content { FirstRun, Library, Playlist };

    void show_content(Content content);
    framework::Rect padded_area() const;
    int card_height(int width) const;
    void layout();
    void scrolled(bool moved);

    framework::Label placeholder_;
    LibraryCard library_;
    PlaylistCard playlist_;
    framework::VerticalScroll scroll_;
    Content content_ = Content::FirstRun;
};

}
