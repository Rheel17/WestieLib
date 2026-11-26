#include "ui/log/LogPanel.h"

#include <string>
#include <utility>

#include "platform/win32/Clipboard.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Rect.h"
#include "ui/log/LogMenu.h"

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
        table_(hwnd(), lines_),
        on_go_to_(std::move(callbacks.on_go_to)) {
    show_lines(true);
}

void LogPanel::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    toolbar_.apply_dpi(dpi, fonts);
    table_.apply_dpi(dpi, fonts.regular());
    layout();
}

void LogPanel::add(const core::LogLine& line) {
    keep_selection();
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
    keep_selection();
    if (lines_.set_filter(std::move(filter))) {
        show_lines(follow);
    } else if (!unshown_lines_) {
        kept_selection_.reset();
    }
}

void LogPanel::rename_track(const std::string& from, const std::string& to) {
    bool follow = table_.shows_last_line();
    keep_selection();
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
    case WM_SIZE:        layout(); return 0;
    case WM_NOTIFY:      return on_notify(*reinterpret_cast<NMHDR*>(lparam));
    case WM_CONTEXTMENU: on_context_menu(reinterpret_cast<HWND>(wparam), lparam); return 0;
    default:             return Panel::handle_message(message, wparam, lparam);
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
    keep_selection();
    lines_.set_filter(std::move(filter));
    show_lines(true);
}

void LogPanel::keep_selection() {
    if (!kept_selection_) {
        kept_selection_ = table_.selection();
    }
}

void LogPanel::show_lines(bool to_last_line) {
    table_.show_lines(to_last_line, std::exchange(kept_selection_, std::nullopt));
    unshown_lines_ = false;
}

LRESULT LogPanel::on_notify(NMHDR& header) {
    if (header.hwndFrom != table_.hwnd()) {
        return 0;
    }
    switch (header.code) {
    case NM_DBLCLK:
        if (int line = reinterpret_cast<const NMITEMACTIVATE&>(header).iItem; line >= 0 && std::cmp_less(line, lines_.shown_count())) {
            go_to(line);
        }
        return 0;
    case LVN_KEYDOWN:
        if (reinterpret_cast<const NMLVKEYDOWN&>(header).wVKey == 'C' && GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_SHIFT) >= 0 && GetKeyState(VK_MENU) >= 0) {
            copy_selection();
        }
        return 0;
    default: return table_.on_notify(header);
    }
}

void LogPanel::on_context_menu(HWND window, LPARAM point) {
    if (window != table_.hwnd()) {
        return;
    }
    std::optional<POINT> screen = table_.menu_point(point);
    std::optional<int> line = table_.focused_line();
    if (!screen || !line) {
        return;
    }
    const core::LogSubject& subject = lines_.shown(*line).subject;
    std::optional<LogLineAction> action = choose_log_line_action(hwnd(), *screen, {.track = !subject.track.empty(), .playlist = !subject.playlist.empty()});
    if (!action) {
        return;
    }
    switch (*action) {
    // A line is about a track or a playlist, never both
    case LogLineAction::GoToTrack:
    case LogLineAction::GoToPlaylist: go_to(*line); return;
    case LogLineAction::Copy:         copy_selection(); return;
    }
}

// A line about neither does nothing
void LogPanel::go_to(int line) {
    core::LogSubject subject = lines_.shown(line).subject;
    if (!subject.track.empty() || !subject.playlist.empty()) {
        on_go_to_(subject);
    }
}

void LogPanel::copy_selection() const {
    std::wstring text = table_.selected_text();
    if (!text.empty() && !platform::win32::set_clipboard_text(hwnd(), text)) {
        MessageBeep(MB_ICONWARNING);
    }
}

}
