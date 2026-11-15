#include "ui/log/LogPanel.h"

#include <utility>

#include "platform/win32/Win32Error.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Rect.h"

namespace westie::ui {

LogPanel::LogPanel(HWND parent, Callbacks callbacks) :
        Panel(parent, L"Log"),
        toolbar_(
                hwnd(),
                {
                    .on_view_change = [this](LogView view) { change_view(view); },
                    .on_open_folder = std::move(callbacks.on_open_folder),
                    .on_close = std::move(callbacks.on_close),
                }
        ),
        table_(hwnd(), lines_) {
    show_lines(true);
}

void LogPanel::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    toolbar_.apply_dpi(dpi, fonts);
    table_.apply_dpi(dpi, fonts.regular());
    layout();
}

void LogPanel::add(const core::LogLine& line) {
    lines_.append(line);
    unshown_lines_ = true;
}

void LogPanel::show_added() {
    // The list still has the count it showed, so it tells whether the newest line was in view
    if (unshown_lines_) {
        show_lines(table_.shows_last_line());
    }
}

void LogPanel::set_selection(LogSelection selection) {
    LogFilter filter = lines_.filter();
    // Another playlist or track shows other lines from their end; the selected playlist's tracks change as it syncs,
    // which keeps the list where it is
    bool other = selection.playlist != filter.selection.playlist || selection.tracks != filter.selection.tracks;
    bool follow = other || table_.shows_last_line();
    filter.selection = std::move(selection);
    if (lines_.set_filter(std::move(filter))) {
        show_lines(follow);
    }
}

void LogPanel::rename_track(const std::string& from, const std::string& to) {
    bool follow = table_.shows_last_line();
    lines_.rename_track(from, to);
    show_lines(follow);
}

LogView LogPanel::view() const {
    return lines_.filter().view;
}

void LogPanel::set_view(LogView view) {
    toolbar_.set_view(view);
    change_view(view);
}

LRESULT LogPanel::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:   layout(); return 0;
    case WM_NOTIFY: return on_notify(*reinterpret_cast<NMHDR*>(lparam));
    default:        return Panel::handle_message(message, wparam, lparam);
    }
}

void LogPanel::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::Rect area = framework::Rect::from_rect(client);
    framework::DeferredPositions positions;
    positions.place(toolbar_.hwnd(), area.take_top(toolbar_.height()));
    positions.place(table_.hwnd(), area);
    positions.apply();
    table_.fit_message_column();
}

void LogPanel::change_view(LogView view) {
    LogFilter filter = lines_.filter();
    filter.view = view;
    lines_.set_filter(std::move(filter));
    show_lines(true);
}

void LogPanel::show_lines(bool to_last_line) {
    table_.show_lines(to_last_line);
    unshown_lines_ = false;
}

LRESULT LogPanel::on_notify(NMHDR& header) {
    if (header.hwndFrom == table_.hwnd()) {
        return table_.on_notify(header);
    }
    return 0;
}

}
