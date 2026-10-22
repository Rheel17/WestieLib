#pragma once

#include <optional>
#include <string>

#include <windows.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Label.h"
#include "ui/framework/Panel.h"
#include "ui/tracks/FilterBar.h"
#include "ui/tracks/StatusFilter.h"
#include "ui/tracks/TrackTable.h"

namespace westie::ui {

class TrackPanel final : public framework::Panel {
public:
    explicit TrackPanel(HWND parent);

    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void show_scope(std::optional<std::wstring> playlist_name);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void layout();
    void paint() const;
    LRESULT on_notify(NMHDR& header);
    void update_clear();
    void clear_status();
    void show_result_line();

    FilterBar filter_;
    StatusFilter status_;
    framework::Label result_;
    TrackTable table_;
    std::optional<std::wstring> playlist_name_;
    int result_height_ = 0;
    int rule_top_ = 0;
};

}
