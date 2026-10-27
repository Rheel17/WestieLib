#pragma once

#include <array>

#include <windows.h>

#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Label.h"
#include "ui/framework/Panel.h"
#include "ui/inspector/IconText.h"
#include "ui/inspector/SectionTitle.h"
#include "ui/tracks/StatusCounts.h"

namespace westie::ui {

class SelectionCard final : public framework::Panel {
public:
    explicit SelectionCard(HWND parent);

    int height_for(int width) const;
    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void set_contents(const StatusCounts& counts);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    struct StatusRow {
        StatusTab status = StatusTab::Synced;
        framework::Label key;
        IconText count;
    };

    StatusRow status_row(StatusTab status);
    int arrange(int width, framework::DeferredPositions* positions) const;
    void layout();

    IconText title_;
    SectionTitle section_;
    std::array<StatusRow, 5> rows_;
    framework::Label all_removed_;
    StatusCounts counts_;
    int line_height_ = 0;
};

}
