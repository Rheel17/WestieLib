#pragma once

#include <functional>
#include <string>

#include <windows.h>
#include <commctrl.h>

#include "core/log/LogLine.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Panel.h"
#include "ui/log/LogFilter.h"
#include "ui/log/LogLines.h"
#include "ui/log/LogTable.h"
#include "ui/log/LogToolbar.h"

namespace westie::ui {

class LogPanel final : public framework::Panel {
public:
    struct Callbacks {
        std::function<void()> on_open_folder;
        std::function<void()> on_close;
    };

    LogPanel(HWND parent, Callbacks callbacks);

    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void add(const core::LogLine& line);
    void set_selection(LogSelection selection);
    void rename_track(const std::string& from, const std::string& to);
    LogView view() const;
    void set_view(LogView view);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void layout();
    void change_view(LogView view);
    LRESULT on_notify(NMHDR& header);

    LogLines lines_;
    LogToolbar toolbar_;
    LogTable table_;
};

}
