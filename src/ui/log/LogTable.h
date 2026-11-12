#pragma once

#include <windows.h>
#include <commctrl.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/ListView.h"
#include "ui/framework/MessageFont.h"
#include "ui/log/LogLines.h"

namespace westie::ui {

class LogTable {
public:
    LogTable(HWND parent, const LogLines& lines);

    HWND hwnd() const noexcept;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    bool shows_last_line() const;
    void show_lines(bool to_last_line);
    void fit_message_column();
    LRESULT on_notify(NMHDR& header);

private:
    void show_cell(LVITEMW& item) const;
    LRESULT on_custom_draw(NMLVCUSTOMDRAW& draw) const;
    void on_empty_markup(NMLVEMPTYMARKUP& markup) const;
    const wchar_t* empty_text() const;

    const LogLines& lines_;
    framework::ListView list_;
};

}
