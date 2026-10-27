#pragma once

#include <filesystem>
#include <string>

#include <windows.h>

#include "ui/TrackCounts.h"
#include "ui/commands/CommandState.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Panel.h"
#include "ui/framework/VerticalScroll.h"
#include "ui/inspector/InspectorEvents.h"
#include "ui/inspector/LibraryCard.h"
#include "ui/inspector/PlaylistCard.h"
#include "ui/inspector/TrackCard.h"
#include "ui/inspector/TrackDetails.h"
#include "ui/playlists/PlaylistRow.h"

namespace westie::ui {

class InspectorPanel final : public framework::Panel {
public:
    InspectorPanel(HWND parent, const InspectorEvents& events);

    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void show_first_run();
    void show_library(const std::filesystem::path& folder, const TrackCounts& counts);
    void show_playlist(const PlaylistRow& playlist);
    void show_track(const TrackDetails& track);
    void update(const CommandState& state);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    enum class Content { FirstRun, Library, Playlist, Track };

    // The track's or playlist's key, if the card shows one
    void show_content(Content content, const std::string& subject);
    framework::Rect padded_area() const;
    int card_height(int width) const;
    void layout();
    void scrolled(bool moved);

    LibraryCard library_;
    PlaylistCard playlist_;
    TrackCard track_;
    framework::VerticalScroll scroll_;
    Content content_ = Content::FirstRun;
    std::string subject_;
};

}
