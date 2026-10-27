#pragma once

#include <filesystem>
#include <functional>

#include <windows.h>

#include "ui/TrackCounts.h"
#include "ui/commands/CommandId.h"
#include "ui/commands/CommandState.h"
#include "ui/framework/Button.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Label.h"
#include "ui/framework/Panel.h"
#include "ui/inspector/IconText.h"
#include "ui/inspector/SectionTitle.h"

namespace westie::ui {

class LibraryCard final : public framework::Panel {
public:
    LibraryCard(HWND parent, std::function<void(CommandId)> on_command);

    int height_for(int width) const;
    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void set_contents(const std::filesystem::path& folder, const TrackCounts& counts);
    void update(const CommandState& state);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    int arrange(int width, framework::DeferredPositions* positions) const;
    void layout();
    void on_command(WPARAM wparam) const;

    std::function<void(CommandId)> on_command_;
    IconText title_;
    SectionTitle section_;
    framework::Label folder_key_;
    framework::Label folder_;
    framework::Label tracks_key_;
    framework::Label tracks_;
    framework::Button open_;
    int line_height_ = 0;
};

}
