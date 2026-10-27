#pragma once

#include <vector>

#include <windows.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/MessageFont.h"
#include "ui/framework/TabStrip.h"
#include "ui/tracks/StatusCounts.h"

namespace westie::ui {

class StatusFilter {
public:
    explicit StatusFilter(HWND parent);

    HWND hwnd() const noexcept;
    int height() const noexcept;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    StatusTab selected() const noexcept;
    void set_counts(const StatusCounts& counts);
    void show_duplicates(bool shown);
    void reset();
    void on_selection_change();

private:
    void show();

    framework::TabStrip tabs_;
    StatusCounts counts_;
    StatusTab selected_ = StatusTab::All;
    bool duplicates_shown_ = false;
    std::vector<StatusTab> shown_;
};

}
