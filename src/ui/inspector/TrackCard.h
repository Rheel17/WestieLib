#pragma once

#include <cstddef>

#include <windows.h>

#include "ui/commands/CommandState.h"
#include "ui/framework/Button.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Label.h"
#include "ui/framework/LinkLabel.h"
#include "ui/framework/Panel.h"
#include "ui/inspector/AudioRows.h"
#include "ui/inspector/CoverArt.h"
#include "ui/inspector/IconText.h"
#include "ui/inspector/InspectorEvents.h"
#include "ui/inspector/OutputBox.h"
#include "ui/inspector/SectionTitle.h"
#include "ui/inspector/SourceLink.h"
#include "ui/inspector/TrackDetails.h"

namespace westie::ui {

class TrackCard final : public framework::Panel {
public:
    TrackCard(HWND parent, InspectorEvents events);

    int height_for(int width) const;
    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void set_contents(const TrackDetails& track);
    void update(const CommandState& state);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    int arrange(int width, framework::DeferredPositions* positions) const;
    void layout();
    void on_command(WPARAM wparam) const;
    void open_source(std::size_t index) const;
    void select_playlist(std::size_t index) const;

    InspectorEvents events_;
    CoverArt cover_;
    framework::Label title_;
    framework::Label artists_;
    framework::Label album_;
    SectionTitle status_section_;
    IconText status_;
    framework::Label explanation_;
    framework::Label review_note_;
    framework::Label reason_;
    OutputBox output_;
    framework::Button confirm_;
    SectionTitle source_section_;
    framework::Label first_source_key_;
    SourceLink first_source_;
    framework::Label second_source_key_;
    SourceLink second_source_;
    framework::Label playlists_key_;
    framework::LinkLabel playlists_;
    framework::Label no_playlists_;
    framework::Label key_key_;
    framework::Label key_;
    AudioRows audio_;
    SectionTitle file_section_;
    framework::Label file_;
    TrackDetails track_;
    int line_height_ = 0;
    int title_line_height_ = 0;
};

}
