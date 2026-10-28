#include "ui/dialogs/VideoListView.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
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

constexpr int ArtistsWidthDips = 130;
constexpr int AlbumWidthDips = 130;
constexpr int LengthWidthDips = 52;
constexpr int ScoreWidthDips = 52;
// Off by more, a video is often a live version or a cover
constexpr std::chrono::milliseconds LengthTolerance{5000};
constexpr double PercentPerWhole = 100.0;

enum class Column { Title, Artists, Album, Length, Score };

std::wstring title_text(const core::VideoCandidate& candidate) {
    std::wstring title = platform::win32::to_utf16(candidate.title);
    if (candidate.current) {
        return title + L" (current)";
    }
    return candidate.track.empty() ? title : title + L" (in library)";
}

bool length_off(std::chrono::milliseconds video, std::chrono::milliseconds source) {
    if (video.count() <= 0 || source.count() <= 0) {
        return false;
    }
    return (video > source ? video - source : source - video) > LengthTolerance;
}

}

VideoListView::VideoListView(HWND list) : list_(list) {
    platform::win32::check_hresult(SetWindowTheme(list_, L"Explorer", nullptr), "SetWindowTheme");
    ListView_SetExtendedListViewStyle(list_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    add_columns();
}

void VideoListView::show(const std::vector<core::VideoCandidate>& candidates, std::chrono::milliseconds source_duration) {
    platform::win32::check(ListView_DeleteAllItems(list_), "LVM_DELETEALLITEMS");
    length_off_.clear();
    for (int item = 0; std::cmp_less(item, candidates.size()); ++item) {
        const core::VideoCandidate& candidate = candidates[item];
        std::wstring title = title_text(candidate);
        LVITEMW row{};
        row.mask = LVIF_TEXT;
        row.iItem = item;
        row.pszText = title.data();
        if (ListView_InsertItem(list_, &row) == -1) {
            platform::win32::throw_last_error("LVM_INSERTITEMW");
        }
        set_text(item, std::to_underlying(Column::Artists), artists_text(candidate.artists));
        set_text(item, std::to_underlying(Column::Album), platform::win32::to_utf16(candidate.album));
        set_text(item, std::to_underlying(Column::Length), candidate.duration.count() > 0 ? duration_text(candidate.duration) : L"");
        set_text(item, std::to_underlying(Column::Score), std::format(L"{}%", std::lround(candidate.score * PercentPerWhole)));
        length_off_.push_back(length_off(candidate.duration, source_duration));
    }
}

void VideoListView::set_empty_text(std::wstring text) {
    empty_text_ = std::move(text);
    // A list view shows no window text, so this is for screen readers and the UI checks
    platform::win32::check(SetWindowTextW(list_, empty_text_.c_str()), "SetWindowTextW");
    platform::win32::check(InvalidateRect(list_, nullptr, TRUE), "InvalidateRect");
}

std::optional<std::size_t> VideoListView::selected() const {
    int item = ListView_GetNextItem(list_, -1, LVNI_SELECTED);
    return item < 0 ? std::nullopt : std::optional(static_cast<std::size_t>(item));
}

LRESULT VideoListView::on_custom_draw(NMLVCUSTOMDRAW& draw) const {
    switch (draw.nmcd.dwDrawStage) {
    case CDDS_PREPAINT:     return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT: return CDRF_NOTIFYSUBITEMDRAW;
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

void VideoListView::add_columns() {
    framework::Dpi dpi = framework::Dpi::of(list_);
    RECT client;
    platform::win32::check(GetClientRect(list_, &client), "GetClientRect");
    int fixed = dpi.scale(ArtistsWidthDips) + dpi.scale(AlbumWidthDips) + dpi.scale(LengthWidthDips) + dpi.scale(ScoreWidthDips);

    struct ColumnSpec {
        const wchar_t* title = nullptr;
        int width = 0;
        int format = LVCFMT_LEFT;
    };
    std::array<ColumnSpec, 5> columns = {{
        {.title = L"Title", .width = client.right - fixed - GetSystemMetrics(SM_CXVSCROLL)},
        {.title = L"Artists", .width = dpi.scale(ArtistsWidthDips)},
        {.title = L"Album", .width = dpi.scale(AlbumWidthDips)},
        {.title = L"Length", .width = dpi.scale(LengthWidthDips), .format = LVCFMT_RIGHT},
        {.title = L"Score", .width = dpi.scale(ScoreWidthDips), .format = LVCFMT_RIGHT},
    }};
    for (int index = 0; std::cmp_less(index, columns.size()); ++index) {
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        column.fmt = columns[index].format;
        column.pszText = const_cast<wchar_t*>(columns[index].title);
        column.cx = columns[index].width;
        if (ListView_InsertColumn(list_, index, &column) == -1) {
            platform::win32::throw_last_error("LVM_INSERTCOLUMNW");
        }
    }
}

void VideoListView::set_text(int item, int column, const std::wstring& text) {
    LVITEMW cell{};
    cell.iSubItem = column;
    cell.pszText = const_cast<wchar_t*>(text.c_str());
    SendMessageW(list_, LVM_SETITEMTEXTW, item, reinterpret_cast<LPARAM>(&cell));
}

}
