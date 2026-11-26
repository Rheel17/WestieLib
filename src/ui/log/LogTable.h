#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <windows.h>
#include <commctrl.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/ListView.h"
#include "ui/framework/MessageFont.h"
#include "ui/log/LogLines.h"

namespace westie::ui {

// By line number, so it stays with the lines when the lines shown change
struct LineSelection {
    std::vector<std::size_t> lines{};
    std::optional<std::size_t> focused{};
};

class LogTable {
public:
    LogTable(HWND parent, const LogLines& lines);

    HWND hwnd() const noexcept;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    bool shows_last_line() const;
    // As the lines were before they changed
    LineSelection selection() const;
    void show_lines(bool to_last_line, const std::optional<LineSelection>& selection);
    void fit_message_column();
    std::optional<POINT> menu_point(LPARAM point);
    // An index into the lines shown
    std::optional<int> focused_line() const;
    // A line for each selected one, its columns as the list shows them, separated by tabs
    std::wstring selected_text() const;
    LRESULT on_notify(NMHDR& header);

private:
    void select(const LineSelection& selection);
    void show_cell(LVITEMW& item) const;
    std::wstring message_tip(int item, int column_index) const;
    LRESULT on_custom_draw(NMLVCUSTOMDRAW& draw);
    void draw_message(HDC dc, int item) const;
    void on_empty_markup(NMLVEMPTYMARKUP& markup) const;
    const wchar_t* empty_text() const;

    const LogLines& lines_;
    framework::ListView list_;
    // The row the list is drawing, whose message the app draws itself
    std::optional<int> drawing_item_;
};

}
