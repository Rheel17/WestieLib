#include "ui/dialogs/CandidateListView.h"

#include <algorithm>
#include <array>
#include <utility>

#include <uxtheme.h>

#include "platform/win32/Utf16.h"
#include "platform/win32/Win32Error.h"
#include "ui/ArtistsText.h"
#include "ui/DurationText.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/TextColor.h"

namespace westie::ui {

namespace {

constexpr int ArtistsWidthDips = 115;
constexpr int AlbumWidthDips = 115;
constexpr int LengthWidthDips = 52;
constexpr int MatchWidthDips = 110;
constexpr int PlayWidthDips = 26;
// What a cell leaves around its text, on both sides together
constexpr int CellPaddingDips = 12;
// DT_MODIFYSTRING writes an ellipsis of up to this many characters past the text
constexpr std::size_t EllipsisRoom = 4;
// Off by more, a result is often a live version or a cover
constexpr std::chrono::milliseconds LengthTolerance{5000};

enum class Column { Title, Artists, Album, Length, Match, Play };

struct ColumnSpec {
    const wchar_t* title = nullptr;
    int width_dips = 0;
    int format = LVCFMT_LEFT;
};

// Title takes the width the others leave
constexpr std::array<ColumnSpec, 6> Columns = {{
    {.title = L"Title"},
    {.title = L"Artists", .width_dips = ArtistsWidthDips},
    {.title = L"Album", .width_dips = AlbumWidthDips},
    {.title = L"Length", .width_dips = LengthWidthDips, .format = LVCFMT_RIGHT},
    {.title = L"Match", .width_dips = MatchWidthDips},
    {.title = L"", .width_dips = PlayWidthDips},
}};

std::wstring title_mark(const core::MatchCandidate& candidate) {
    if (candidate.current) {
        return L"(current)";
    }
    return candidate.track.empty() ? L"" : L"(in library)";
}

int text_width(HDC dc, const std::wstring& text) {
    SIZE size{};
    platform::win32::check(GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size), "GetTextExtentPoint32W");
    return size.cx;
}

void draw_text(HDC dc, const std::wstring& text, RECT area, COLORREF color) {
    SetTextColor(dc, color);
    DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &area, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
}

// The text DrawTextW would show in a width, with its ellipsis
std::wstring fitted_text(HDC dc, const std::wstring& text, int width) {
    std::wstring fitted = text + std::wstring(EllipsisRoom, L'\0');
    RECT area{0, 0, width, 0};
    DrawTextW(dc, fitted.data(), static_cast<int>(text.size()), &area, DT_SINGLELINE | DT_END_ELLIPSIS | DT_MODIFYSTRING | DT_NOPREFIX | DT_CALCRECT);
    if (std::size_t end = fitted.find(L'\0'); end != std::wstring::npos) {
        fitted.resize(end);
    }
    return fitted;
}

const wchar_t* doubt_word(core::MatchDoubt doubt) {
    switch (doubt) {
    case core::MatchDoubt::Title:    return L"title";
    case core::MatchDoubt::Artist:   return L"artist";
    case core::MatchDoubt::Version:  return L"version";
    case core::MatchDoubt::Length:   return L"length";
    case core::MatchDoubt::Explicit: return L"explicit";
    }
    return L"";
}

std::wstring verdict_text(const core::MatchCandidate& candidate) {
    if (candidate.verdict == core::MatchVerdict::Accept) {
        return L"Same recording";
    }
    if (candidate.verdict == core::MatchVerdict::Review) {
        std::wstring doubts;
        for (core::MatchDoubt doubt : candidate.doubts) {
            doubts += (doubts.empty() ? L"" : L", ") + std::wstring(doubt_word(doubt));
        }
        return L"Check " + doubts;
    }
    switch (candidate.mismatch) {
    case core::MatchMismatch::Recording: return L"Other recording";
    case core::MatchMismatch::Artist:    return L"Other artist";
    default:                             return L"Other song";
    }
}

bool length_off(std::chrono::milliseconds result, std::chrono::milliseconds source) {
    if (result.count() <= 0 || source.count() <= 0) {
        return false;
    }
    return (result > source ? result - source : source - result) > LengthTolerance;
}

}

CandidateListView::CandidateListView(HWND list, std::function<void(std::size_t)> on_play) :
        Control(list), play_(list, std::to_underlying(Column::Play), std::move(on_play)) {
    platform::win32::check_hresult(SetWindowTheme(hwnd(), L"Explorer", nullptr), "SetWindowTheme");
    ListView_SetExtendedListViewStyle(hwnd(), LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    add_columns();
}

void CandidateListView::show(const std::vector<core::MatchCandidate>& candidates, std::chrono::milliseconds source_duration) {
    platform::win32::check(ListView_DeleteAllItems(hwnd()), "LVM_DELETEALLITEMS");
    titles_.clear();
    length_off_.clear();
    for (int item = 0; std::cmp_less(item, candidates.size()); ++item) {
        const core::MatchCandidate& candidate = candidates[item];
        titles_.push_back({.text = platform::win32::to_utf16(candidate.title), .mark = title_mark(candidate)});
        LVITEMW row{};
        row.mask = LVIF_TEXT;
        row.iItem = item;
        // The list asks for it, so it gets no title while it draws a row (show_cell)
        row.pszText = LPSTR_TEXTCALLBACKW;
        if (ListView_InsertItem(hwnd(), &row) == -1) {
            platform::win32::throw_last_error("LVM_INSERTITEMW");
        }
        set_text(item, std::to_underlying(Column::Artists), artists_text(candidate.artists));
        set_text(item, std::to_underlying(Column::Album), platform::win32::to_utf16(candidate.album));
        set_text(item, std::to_underlying(Column::Length), candidate.duration.count() > 0 ? duration_text(candidate.duration) : L"");
        set_text(item, std::to_underlying(Column::Match), verdict_text(candidate));
        length_off_.push_back(length_off(candidate.duration, source_duration));
    }
}

void CandidateListView::set_empty_text(std::wstring text) {
    empty_text_ = std::move(text);
    // A list view shows no window text, so this is for screen readers and the UI checks
    platform::win32::check(SetWindowTextW(hwnd(), empty_text_.c_str()), "SetWindowTextW");
    platform::win32::check(InvalidateRect(hwnd(), nullptr, TRUE), "InvalidateRect");
}

std::optional<std::size_t> CandidateListView::selected() const {
    int item = ListView_GetNextItem(hwnd(), -1, LVNI_SELECTED);
    return item < 0 ? std::nullopt : std::optional(static_cast<std::size_t>(item));
}

// The title and its mark, for screen readers and the UI checks; while the list draws the row, nothing, so it draws the
// cell's background and selection and the app draws the title over it
void CandidateListView::show_cell(LVITEMW& item) const {
    if (!(item.mask & LVIF_TEXT) || item.cchTextMax <= 0 || item.iSubItem != std::to_underlying(Column::Title)) {
        return;
    }
    std::wstring text;
    if (item.iItem >= 0 && std::cmp_less(item.iItem, titles_.size()) && drawing_item_ != item.iItem) {
        const Title& title = titles_[item.iItem];
        text = title.mark.empty() ? title.text : title.text + L" " + title.mark;
    }
    std::size_t length = std::min(text.size(), static_cast<std::size_t>(item.cchTextMax - 1));
    std::copy_n(text.begin(), length, item.pszText);
    item.pszText[length] = L'\0';
}

LRESULT CandidateListView::on_custom_draw(NMLVCUSTOMDRAW& draw) {
    auto item = static_cast<int>(draw.nmcd.dwItemSpec);
    switch (draw.nmcd.dwDrawStage) {
    case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW;
    // The list asks for a row's texts after this
    case CDDS_ITEMPREPAINT:
        drawing_item_ = item;
        return CDRF_NOTIFYSUBITEMDRAW | CDRF_NOTIFYPOSTPAINT;
    // The list also runs a row's stages to measure it, with an empty rect, and then draws nothing
    case CDDS_ITEMPOSTPAINT:
        drawing_item_.reset();
        if (!IsRectEmpty(&draw.nmcd.rc)) {
            draw_title(draw.nmcd.hdc, item);
        }
        play_.paint(draw.nmcd.hdc, item);
        return CDRF_DODEFAULT;
    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
        std::size_t item = draw.nmcd.dwItemSpec;
        bool amber = draw.iSubItem == std::to_underlying(Column::Length) && item < length_off_.size() && length_off_[item];
        // Every cell sets its color, or the amber would carry on into the next
        draw.clrText = amber ? framework::text_color(framework::TextColor::Warning) : GetSysColor(COLOR_WINDOWTEXT);
        return CDRF_DODEFAULT;
    }
    default: return CDRF_DODEFAULT;
    }
}

void CandidateListView::on_empty_markup(NMLVEMPTYMARKUP& markup) const {
    markup.dwFlags = EMF_CENTERED;
    std::size_t size = std::min(empty_text_.size(), std::size(markup.szMarkup) - 1);
    empty_text_.copy(markup.szMarkup, size);
    markup.szMarkup[size] = L'\0';
}

LRESULT CandidateListView::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    if (std::optional<LRESULT> result = play_.on_mouse(message, lparam)) {
        return *result;
    }
    LRESULT result = Control::handle_message(message, wparam, lparam);
    switch (message) {
    case WM_SIZE: fit_columns(); return result;
    case WM_DPICHANGED_AFTERPARENT:
        play_.apply_dpi();
        fit_columns();
        return result;
    // The rows move under the mouse
    case WM_VSCROLL:
    case WM_MOUSEWHEEL: play_.follow_cursor(); return result;
    default:            return result;
    }
}

void CandidateListView::add_columns() {
    for (int index = 0; std::cmp_less(index, Columns.size()); ++index) {
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_FMT;
        column.fmt = Columns[index].format;
        column.pszText = const_cast<wchar_t*>(Columns[index].title);
        if (ListView_InsertColumn(hwnd(), index, &column) == -1) {
            platform::win32::throw_last_error("LVM_INSERTCOLUMNW");
        }
    }
    fit_columns();
}

void CandidateListView::fit_columns() {
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    // Room for a scroll bar stays, so rows that come later don't bring a horizontal one along
    int rest = client.right - GetSystemMetricsForDpi(SM_CXVSCROLL, dpi.value());
    for (int index = 1; std::cmp_less(index, Columns.size()); ++index) {
        int width = dpi.scale(Columns[index].width_dips);
        platform::win32::check(ListView_SetColumnWidth(hwnd(), index, width), "LVM_SETCOLUMNWIDTH");
        rest -= width;
    }
    platform::win32::check(ListView_SetColumnWidth(hwnd(), std::to_underlying(Column::Title), std::max(rest, 0)), "LVM_SETCOLUMNWIDTH");
}

void CandidateListView::draw_title(HDC dc, int item) const {
    if (item < 0 || std::cmp_greater_equal(item, titles_.size())) {
        return;
    }
    const Title& title = titles_[item];
    // The first column's label starts a few pixels into it; the row's bounds start where the column does
    RECT cell{LVIR_LABEL, std::to_underlying(Column::Title), 0, 0};
    platform::win32::check(SendMessageW(hwnd(), LVM_GETSUBITEMRECT, item, reinterpret_cast<LPARAM>(&cell)), "LVM_GETSUBITEMRECT");
    RECT row{LVIR_BOUNDS, 0, 0, 0};
    platform::win32::check(SendMessageW(hwnd(), LVM_GETITEMRECT, item, reinterpret_cast<LPARAM>(&row)), "LVM_GETITEMRECT");
    cell.left = row.left;
    int padding = framework::Dpi::of(hwnd()).scale(CellPaddingDips) / 2;
    cell.left += padding;
    cell.right -= padding;

    HGDIOBJ font = SelectObject(dc, reinterpret_cast<HFONT>(SendMessageW(hwnd(), WM_GETFONT, 0, 0)));
    SetBkMode(dc, TRANSPARENT);
    // The mark stays whole while it fits; the title gives way to it
    std::wstring mark = title.mark.empty() ? L"" : L" " + title.mark;
    int room = static_cast<int>(cell.right - cell.left) - text_width(dc, mark);
    std::wstring shown = mark.empty() ? title.text : fitted_text(dc, title.text, std::max(room, 0));
    draw_text(dc, shown, cell, GetSysColor(COLOR_WINDOWTEXT));
    if (!mark.empty()) {
        RECT after = cell;
        after.left = std::min(cell.left + text_width(dc, shown), cell.right);
        draw_text(dc, mark, after, framework::text_color(framework::TextColor::Gray));
    }
    SelectObject(dc, font);
}

void CandidateListView::set_text(int item, int column, const std::wstring& text) {
    LVITEMW cell{};
    cell.iSubItem = column;
    cell.pszText = const_cast<wchar_t*>(text.c_str());
    SendMessageW(hwnd(), LVM_SETITEMTEXTW, item, reinterpret_cast<LPARAM>(&cell));
}

}
