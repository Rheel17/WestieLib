#pragma once

#include <functional>
#include <optional>
#include <string>

#include <windows.h>
#include <commctrl.h>

#include "core/log/LogLine.h"
#include "core/log/LogSubject.h"
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
        std::function<void()> on_open_folder{};
        std::function<void()> on_close{};
        // A line's track or playlist, for the window to select
        std::function<void(const core::LogSubject&)> on_go_to{};
    };

    LogPanel(HWND parent, Callbacks callbacks);

    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    // Lines added show at the next show_added, so a burst of them updates the list once
    void add(const core::LogLine& line);
    void show_added();
    void set_selection(LogSelection selection);
    void rename_track(const std::string& from, const std::string& to);
    LogView view() const;
    void set_view(LogView view);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void layout();
    void change_view(LogView view);
    void keep_selection();
    void show_lines(bool to_last_line);
    LRESULT on_notify(NMHDR& header);
    void on_context_menu(HWND window, LPARAM point);
    void go_to(int line);
    void copy_selection() const;

    LogLines lines_;
    LogToolbar toolbar_;
    LogTable table_;
    std::function<void(const core::LogSubject&)> on_go_to_;
    bool unshown_lines_ = false;
    // The list's selection as it was before the lines changed, until it shows them
    std::optional<LineSelection> kept_selection_;
};

}
