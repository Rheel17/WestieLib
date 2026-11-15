#include "ui/dialogs/VideoListView.h"

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
// Off by more, a video is often a live version or a cover
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

std::wstring title_text(const core::VideoCandidate& candidate) {
    std::wstring title = platform::win32::to_utf16(candidate.title);
    if (candidate.current) {
        return title + L" (current)";
    }
    return candidate.track.empty() ? title : title + L" (in library)";
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

std::wstring verdict_text(const core::VideoCandidate& candidate) {
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

bool length_off(std::chrono::milliseconds video, std::chrono::milliseconds source) {
    if (video.count() <= 0 || source.count() <= 0) {
        return false;
    }
    return (video > source ? video - source : source - video) > LengthTolerance;
}

}

VideoListView::VideoListView(HWND list, std::function<void(std::size_t)> on_play) :
        Control(list), play_(list, std::to_underlying(Column::Play), std::move(on_play)) {
    platform::win32::check_hresult(SetWindowTheme(hwnd(), L"Explorer", nullptr), "SetWindowTheme");
    ListView_SetExtendedListViewStyle(hwnd(), LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    add_columns();
}

void VideoListView::show(const std::vector<core::VideoCandidate>& candidates, std::chrono::milliseconds source_duration) {
    platform::win32::check(ListView_DeleteAllItems(hwnd()), "LVM_DELETEALLITEMS");
    length_off_.clear();
    for (int item = 0; std::cmp_less(item, candidates.size()); ++item) {
        const core::VideoCandidate& candidate = candidates[item];
        std::wstring title = title_text(candidate);
        LVITEMW row{};
        row.mask = LVIF_TEXT;
        row.iItem = item;
        row.pszText = title.data();
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

void VideoListView::set_empty_text(std::wstring text) {
    empty_text_ = std::move(text);
    // A list view shows no window text, so this is for screen readers and the UI checks
    platform::win32::check(SetWindowTextW(hwnd(), empty_text_.c_str()), "SetWindowTextW");
    platform::win32::check(InvalidateRect(hwnd(), nullptr, TRUE), "InvalidateRect");
}

std::optional<std::size_t> VideoListView::selected() const {
    int item = ListView_GetNextItem(hwnd(), -1, LVNI_SELECTED);
    return item < 0 ? std::nullopt : std::optional(static_cast<std::size_t>(item));
}

LRESULT VideoListView::on_custom_draw(NMLVCUSTOMDRAW& draw) const {
    switch (draw.nmcd.dwDrawStage) {
    case CDDS_PREPAINT:      return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:  return CDRF_NOTIFYSUBITEMDRAW | CDRF_NOTIFYPOSTPAINT;
    case CDDS_ITEMPOSTPAINT: play_.paint(draw.nmcd.hdc, static_cast<int>(draw.nmcd.dwItemSpec)); return CDRF_DODEFAULT;
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

void VideoListView::on_empty_markup(NMLVEMPTYMARKUP& markup) const {
    markup.dwFlags = EMF_CENTERED;
    std::size_t size = std::min(empty_text_.size(), std::size(markup.szMarkup) - 1);
    empty_text_.copy(markup.szMarkup, size);
    markup.szMarkup[size] = L'\0';
}

LRESULT VideoListView::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
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

void VideoListView::add_columns() {
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

void VideoListView::fit_columns() {
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

void VideoListView::set_text(int item, int column, const std::wstring& text) {
    LVITEMW cell{};
    cell.iSubItem = column;
    cell.pszText = const_cast<wchar_t*>(text.c_str());
    SendMessageW(hwnd(), LVM_SETITEMTEXTW, item, reinterpret_cast<LPARAM>(&cell));
}

}
