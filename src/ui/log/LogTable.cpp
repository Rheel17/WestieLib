#include "ui/log/LogTable.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "platform/win32/Win32Error.h"
#include "ui/framework/TextColor.h"

namespace westie::ui {

namespace {

constexpr int RowHeightDips = 20;
constexpr int MessageMinDips = 100;
// What a cell leaves around its text, on both sides together
constexpr int CellPaddingDips = 12;
constexpr wchar_t NoTrackSelected[] = L"Select a track to see its lines.";
// LVM_RESETEMPTYTEXT, which MinGW's commctrl.h leaves out
constexpr UINT ResetEmptyTextMessage = LVM_FIRST + 84;

enum class LogColumn { Time, Level, Subject, Message };

struct ColumnSpec {
    LogColumn column = LogColumn::Message;
    const wchar_t* title = nullptr;
    int width_dips = 0;
};

constexpr std::array Columns = {
    ColumnSpec{LogColumn::Time, L"Time", 150},
    ColumnSpec{LogColumn::Level, L"Level", 70},
    ColumnSpec{LogColumn::Subject, L"Playlist / Track", 160},
    // Takes the rest of the width
    ColumnSpec{LogColumn::Message, L"Message", MessageMinDips},
};

const std::wstring& cell_text(const LogRow& row, LogColumn column) {
    switch (column) {
    case LogColumn::Time:    return row.time;
    case LogColumn::Level:   return row.level_text;
    case LogColumn::Subject: return row.subject_text;
    case LogColumn::Message: return row.text;
    }
    return row.text;
}

COLORREF level_color(core::LogLevel level) {
    switch (level) {
    case core::LogLevel::Warning: return framework::text_color(framework::TextColor::Warning);
    case core::LogLevel::Error:   return framework::text_color(framework::TextColor::Error);
    default:                      return framework::text_color(framework::TextColor::Normal);
    }
}

}

LogTable::LogTable(HWND parent, const LogLines& lines) : lines_(lines), list_(parent) {
    framework::Dpi dpi = framework::Dpi::of(list_.hwnd());
    for (const ColumnSpec& spec : Columns) {
        list_.insert_column(list_.column_count(), {.title = spec.title, .width = dpi.scale(spec.width_dips), .subitem = std::to_underlying(spec.column)});
    }
    list_.set_cell_tip([this](int item, int column_index) { return message_tip(item, column_index); });
}

HWND LogTable::hwnd() const noexcept {
    return list_.hwnd();
}

void LogTable::apply_dpi(framework::Dpi dpi, const framework::MessageFont& font) {
    list_.apply_dpi(dpi, font, RowHeightDips);
    fit_message_column();
}

bool LogTable::shows_last_line() const {
    // A hidden pane has no lines in view, and comes back at the newest
    if (!IsWindowVisible(list_.hwnd())) {
        return true;
    }
    auto count = static_cast<int>(SendMessageW(list_.hwnd(), LVM_GETITEMCOUNT, 0, 0));
    auto top = static_cast<int>(SendMessageW(list_.hwnd(), LVM_GETTOPINDEX, 0, 0));
    auto per_page = static_cast<int>(SendMessageW(list_.hwnd(), LVM_GETCOUNTPERPAGE, 0, 0));
    return top + per_page >= count;
}

void LogTable::show_lines(bool to_last_line) {
    auto count = static_cast<int>(lines_.shown_count());
    list_.set_item_count(count);
    // What it reads while it has no lines, as its window text too, so the UI checks can read it
    platform::win32::check(SetWindowTextW(list_.hwnd(), empty_text()), "SetWindowTextW");
    // A scroll bar may have come or gone
    fit_message_column();
    if (to_last_line && count > 0) {
        // While it draws, the list animates the scroll and paints every step of it, which took about 8 ms a line
        SendMessageW(list_.hwnd(), WM_SETREDRAW, FALSE, 0);
        SendMessageW(list_.hwnd(), LVM_ENSUREVISIBLE, count - 1, FALSE);
        SendMessageW(list_.hwnd(), WM_SETREDRAW, TRUE, 0);
    }
    // The list keeps the empty text it was given until told to ask again
    SendMessageW(list_.hwnd(), ResetEmptyTextMessage, 0, 0);
    // A virtual list doesn't know its texts changed
    platform::win32::check(InvalidateRect(list_.hwnd(), nullptr, FALSE), "InvalidateRect");
}

void LogTable::fit_message_column() {
    std::optional<int> message = list_.column_index(std::to_underlying(LogColumn::Message));
    if (!message) {
        return;
    }
    RECT client;
    platform::win32::check(GetClientRect(list_.hwnd(), &client), "GetClientRect");
    int others = 0;
    for (int index = 0; index < list_.column_count(); ++index) {
        if (index != *message) {
            others += list_.column_width(index);
        }
    }
    int width = std::max(framework::Dpi::of(list_.hwnd()).scale(MessageMinDips), static_cast<int>(client.right) - others);
    if (width != list_.column_width(*message)) {
        list_.set_column_width(*message, width);
    }
    list_.set_cell_tip_width(width);
}

LRESULT LogTable::on_notify(NMHDR& header) {
    switch (header.code) {
    case LVN_GETDISPINFOW:   show_cell(reinterpret_cast<NMLVDISPINFOW&>(header).item); return 0;
    case NM_CUSTOMDRAW:      return on_custom_draw(reinterpret_cast<NMLVCUSTOMDRAW&>(header));
    case LVN_GETEMPTYMARKUP: on_empty_markup(reinterpret_cast<NMLVEMPTYMARKUP&>(header)); return TRUE;
    default:                 return 0;
    }
}

void LogTable::show_cell(LVITEMW& item) const {
    if (!(item.mask & LVIF_TEXT) || item.cchTextMax <= 0) {
        return;
    }
    std::wstring_view text;
    if (item.iItem >= 0 && std::cmp_less(item.iItem, lines_.shown_count())) {
        text = cell_text(lines_.shown(item.iItem), static_cast<LogColumn>(list_.column_subitem(item.iSubItem)));
    }
    // The list's buffer is all the room there is; the list ends a cut text in an ellipsis itself
    std::size_t length = std::min(text.size(), static_cast<std::size_t>(item.cchTextMax - 1));
    std::copy_n(text.begin(), length, item.pszText);
    item.pszText[length] = L'\0';
}

// Only a message the column cuts off, such as a tool's command line
std::wstring LogTable::message_tip(int item, int column_index) const {
    if (list_.column_subitem(column_index) != std::to_underlying(LogColumn::Message) || item < 0 || std::cmp_greater_equal(item, lines_.shown_count())) {
        return {};
    }
    const std::wstring& text = lines_.shown(item).text;
    int room = list_.column_width(column_index) - framework::Dpi::of(list_.hwnd()).scale(CellPaddingDips);
    return list_.text_width(text) > room ? text : std::wstring();
}

LRESULT LogTable::on_custom_draw(NMLVCUSTOMDRAW& draw) const {
    auto item = static_cast<int>(draw.nmcd.dwItemSpec);
    bool known = item >= 0 && std::cmp_less(item, lines_.shown_count());
    switch (draw.nmcd.dwDrawStage) {
    case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW;
    // The colors hold for one cell at a time
    case CDDS_ITEMPREPAINT:
    case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
        if (!known) {
            return CDRF_DODEFAULT;
        }
        draw.clrText = level_color(lines_.shown(item).level);
        if (draw.nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
            return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
        }
        // The list has drawn the row's background already; only the text is left to draw
        if (list_.column_subitem(draw.iSubItem) == std::to_underlying(LogColumn::Message)) {
            draw_message(draw.nmcd.hdc, item, draw.iSubItem);
            return CDRF_SKIPDEFAULT;
        }
        return CDRF_NEWFONT;
    default: return CDRF_DODEFAULT;
    }
}

// The list cuts a cell's text at 259 characters, which leaves much of a wide column empty
void LogTable::draw_message(HDC dc, int item, int column_index) const {
    const LogRow& row = lines_.shown(item);
    RECT cell = list_.cell_rect(item, column_index);
    int padding = framework::Dpi::of(list_.hwnd()).scale(CellPaddingDips) / 2;
    cell.left += padding;
    cell.right -= padding;
    HGDIOBJ font = SelectObject(dc, reinterpret_cast<HFONT>(SendMessageW(list_.hwnd(), WM_GETFONT, 0, 0)));
    SetTextColor(dc, level_color(row.level));
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, row.text.c_str(), static_cast<int>(row.text.size()), &cell, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(dc, font);
}

void LogTable::on_empty_markup(NMLVEMPTYMARKUP& markup) const {
    markup.dwFlags = EMF_CENTERED;
    std::wstring_view text = empty_text();
    std::size_t size = std::min(text.size(), std::size(markup.szMarkup) - 1);
    text.copy(markup.szMarkup, size);
    markup.szMarkup[size] = L'\0';
}

const wchar_t* LogTable::empty_text() const {
    const LogFilter& filter = lines_.filter();
    return filter.view.scope == LogScope::Track && filter.selection.tracks.empty() ? NoTrackSelected : L"";
}

}
