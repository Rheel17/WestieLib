#include "ui/tracks/TrackTable.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/DurationText.h"
#include "ui/tracks/RowStatusText.h"

namespace westie::ui {

namespace {

constexpr int RowHeightDips = 22;
constexpr int StatusIconDips = 16;
constexpr int StatusIconLeftDips = 9;
// A negative width asks the list to size the column itself, and a huge one overflows when scaled
constexpr int MaxColumnWidthDips = 4000;

struct ColumnSpec {
    TrackColumn column = TrackColumn::Title;
    const wchar_t* title = nullptr;
    int width_dips = 0;
    bool right_aligned = false;
    bool shown = true;
};

struct StatusIconSpec {
    // None for a duplicate
    std::optional<StatusTab> status{};
    int resource_id = 0;
};

// A position only means something inside a playlist, so # shows only in a playlist scope, always first
constexpr ColumnSpec NumberColumn{.column = TrackColumn::Number, .title = L"#", .width_dips = 48, .right_aligned = true};

// In their default order
constexpr std::array Columns = {
    ColumnSpec{.column = TrackColumn::Status, .title = L"Status", .width_dips = 56},
    ColumnSpec{.column = TrackColumn::Title, .title = L"Title", .width_dips = 204},
    ColumnSpec{.column = TrackColumn::Artists, .title = L"Artists", .width_dips = 156},
    ColumnSpec{.column = TrackColumn::Album, .title = L"Album", .width_dips = 132},
    ColumnSpec{.column = TrackColumn::Playlists, .title = L"Playlists", .width_dips = 114},
    ColumnSpec{.column = TrackColumn::Time, .title = L"Time", .width_dips = 48, .right_aligned = true},
    ColumnSpec{.column = TrackColumn::Source, .title = L"Source", .width_dips = 60, .shown = false},
    ColumnSpec{.column = TrackColumn::Match, .title = L"Match", .width_dips = 70, .shown = false},
    ColumnSpec{.column = TrackColumn::File, .title = L"File", .width_dips = 200, .shown = false},
};

// The statuses with an icon so far; the others come with their features
constexpr std::array StatusIcons = {
    StatusIconSpec{StatusTab::Synced, IDI_SYNCED},
    StatusIconSpec{StatusTab::Waiting, IDI_WAITING},
    StatusIconSpec{StatusTab::NeedsReview, IDI_WARN},
    StatusIconSpec{StatusTab::Failed, IDI_FAILED},
    StatusIconSpec{std::nullopt, IDI_DUPLICATE},
};

std::map<TrackColumn, int> default_widths() {
    std::map<TrackColumn, int> widths{{NumberColumn.column, NumberColumn.width_dips}};
    for (const ColumnSpec& spec : Columns) {
        widths[spec.column] = spec.width_dips;
    }
    return widths;
}

framework::ListColumn list_column(const ColumnSpec& spec, int width) {
    return {
        .title = spec.title,
        .width = width,
        .subitem = std::to_underlying(spec.column),
        .right_aligned = spec.right_aligned,
    };
}

TrackSort default_sort(bool playlist) {
    return TrackSort(playlist ? TrackColumn::Number : TrackColumn::Title);
}

// The Status column shows only its icon
std::wstring cell_text(const TrackRow& row, TrackColumn column) {
    switch (column) {
    case TrackColumn::Number:    return std::format(L"{}", row.position);
    case TrackColumn::Title:     return row.title;
    case TrackColumn::Artists:   return row.artists;
    case TrackColumn::Album:     return row.album;
    case TrackColumn::Playlists: return row.playlists;
    case TrackColumn::Time:      return duration_text(row.duration);
    default:                     return {};
    }
}

}

TrackTable::TrackTable(HWND parent, std::function<void()> on_selection_change) :
        on_selection_change_(std::move(on_selection_change)), width_dips_(default_widths()), list_(parent), sort_(default_sort(false)) {
    framework::Dpi dpi = framework::Dpi::of(list_.hwnd());
    std::vector<int> subitems;
    for (const ColumnSpec& spec : Columns) {
        if (spec.shown) {
            list_.insert_column(spec.column == TrackColumn::Title ? 0 : list_.column_count(), list_column(spec, dpi.scale(spec.width_dips)));
            subitems.push_back(std::to_underlying(spec.column));
        }
    }
    std::vector<int> order;
    for (int subitem : subitems) {
        order.push_back(*list_.column_index(subitem));
    }
    list_.set_column_order(order);
    show_sort();
    list_.set_cell_tip([this](int item, int column_index) { return cell_tip(item, column_index); });
}

HWND TrackTable::hwnd() const noexcept {
    return list_.hwnd();
}

void TrackTable::apply_dpi(framework::Dpi dpi, const framework::MessageFont& font) {
    list_.apply_dpi(dpi, font, RowHeightDips);
    status_icons_.clear();
    for (const StatusIconSpec& spec : StatusIcons) {
        status_icons_.push_back({.status = spec.status, .icon = framework::IconResource(spec.resource_id, dpi.scale(StatusIconDips))});
    }
}

void TrackTable::show_scope(bool playlist) {
    std::optional<int> number = list_.column_index(std::to_underlying(TrackColumn::Number));
    if (playlist && !number) {
        add_number_column();
    } else if (!playlist && number) {
        width_dips_ = column_widths();
        list_.delete_column(*number);
    }
    sort_ = default_sort(playlist);
    show_sort();
}

std::map<TrackColumn, int> TrackTable::column_widths() const {
    std::map<TrackColumn, int> widths = width_dips_;
    framework::Dpi dpi = framework::Dpi::of(list_.hwnd());
    for (int index = 0; index < list_.column_count(); ++index) {
        widths[static_cast<TrackColumn>(list_.column_subitem(index))] = dpi.unscale(list_.column_width(index));
    }
    return widths;
}

void TrackTable::set_column_widths(const std::map<TrackColumn, int>& width_dips) {
    for (auto [column, width] : width_dips) {
        width_dips_[column] = std::clamp(width, 0, MaxColumnWidthDips);
    }
    framework::Dpi dpi = framework::Dpi::of(list_.hwnd());
    for (int index = 0; index < list_.column_count(); ++index) {
        list_.set_column_width(index, dpi.scale(width_dips_.at(static_cast<TrackColumn>(list_.column_subitem(index)))));
    }
}

void TrackTable::set_rows(std::vector<TrackRow> rows) {
    Selection selected = selection();
    rows_ = std::move(rows);
    sort_.sort(rows_);
    list_.set_item_count(static_cast<int>(rows_.size()));
    restore(selected);
    // A virtual list doesn't know its texts changed
    platform::win32::check(InvalidateRect(list_.hwnd(), nullptr, FALSE), "InvalidateRect");
}

void TrackTable::scroll_to_top() {
    list_.scroll_to_top();
}

void TrackTable::clear_selection() {
    list_.select_items({}, std::nullopt);
}

std::vector<TrackRow> TrackTable::selected_rows() const {
    std::vector<TrackRow> selected;
    for (int item : list_.selected_items()) {
        if (std::cmp_less(item, rows_.size())) {
            selected.push_back(rows_[item]);
        }
    }
    return selected;
}

LRESULT TrackTable::on_notify(NMHDR& header) {
    switch (header.code) {
    case LVN_COLUMNCLICK:    sort_by(reinterpret_cast<const NMLISTVIEW&>(header).iSubItem); return 0;
    case LVN_GETDISPINFOW:   show_cell(reinterpret_cast<NMLVDISPINFOW&>(header).item); return 0;
    case LVN_ITEMCHANGED:    on_item_changed(reinterpret_cast<const NMLISTVIEW&>(header)); return 0;
    // A virtual list reports a range selected with Shift only as the range
    case LVN_ODSTATECHANGED: on_selection_change_(); return 0;
    case NM_CUSTOMDRAW:      return on_custom_draw(reinterpret_cast<const NMLVCUSTOMDRAW&>(header));
    default:                 return 0;
    }
}

void TrackTable::on_item_changed(const NMLISTVIEW& change) const {
    if ((change.uChanged & LVIF_STATE) && ((change.uNewState ^ change.uOldState) & LVIS_SELECTED)) {
        on_selection_change_();
    }
}

void TrackTable::add_number_column() {
    int index = list_.column_count();
    list_.insert_column(index, list_column(NumberColumn, framework::Dpi::of(list_.hwnd()).scale(width_dips_.at(TrackColumn::Number))));
    std::vector<int> order = list_.column_order();
    std::erase(order, index);
    order.insert(order.begin(), index);
    list_.set_column_order(order);
}

void TrackTable::sort_by(int column_index) {
    sort_.sort_by(static_cast<TrackColumn>(list_.column_subitem(column_index)));
    show_sort();

    Selection selected = selection();
    sort_.sort(rows_);
    restore(selected);
    // The row the user was on moved with the sort, so the list follows it
    if (std::optional<int> focused = list_.focused_item()) {
        SendMessageW(list_.hwnd(), LVM_ENSUREVISIBLE, *focused, FALSE);
    }
    platform::win32::check(InvalidateRect(list_.hwnd(), nullptr, FALSE), "InvalidateRect");
}

void TrackTable::show_sort() {
    std::optional<int> index = list_.column_index(std::to_underlying(sort_.column()));
    list_.set_sort_arrow(index ? std::optional(framework::ListSortArrow{.index = *index, .direction = sort_.direction()}) : std::nullopt);
}

TrackTable::Selection TrackTable::selection() const {
    Selection selected;
    for (int item : list_.selected_items()) {
        if (std::cmp_less(item, rows_.size())) {
            selected.keys.push_back(rows_[item].key);
        }
    }
    if (std::optional<int> focused = list_.focused_item(); focused && std::cmp_less(*focused, rows_.size())) {
        selected.focused = rows_[*focused].key;
    }
    return selected;
}

void TrackTable::restore(const Selection& selection) {
    std::set<std::string_view> keys(selection.keys.begin(), selection.keys.end());
    std::vector<int> items;
    std::optional<int> focused;
    for (std::size_t index = 0; index < rows_.size(); ++index) {
        if (keys.contains(rows_[index].key)) {
            items.push_back(static_cast<int>(index));
        }
        if (selection.focused == rows_[index].key) {
            focused = static_cast<int>(index);
        }
    }
    list_.select_items(items, focused);
}

void TrackTable::show_cell(LVITEMW& item) const {
    if (!(item.mask & LVIF_TEXT) || item.cchTextMax <= 0) {
        return;
    }
    std::wstring text;
    if (item.iItem >= 0 && std::cmp_less(item.iItem, rows_.size())) {
        // The list asks by the column's index, which changes as # comes and goes
        text = cell_text(rows_[item.iItem], static_cast<TrackColumn>(list_.column_subitem(item.iSubItem)));
    }
    // The list's buffer is all the room there is; the list ends a cut text in an ellipsis itself
    std::size_t length = std::min(text.size(), static_cast<std::size_t>(item.cchTextMax - 1));
    std::copy_n(text.begin(), length, item.pszText);
    item.pszText[length] = L'\0';
}

std::wstring TrackTable::cell_tip(int item, int column_index) const {
    // The Status column shows only an icon, so its tooltip gives the word
    if (item < 0 || std::cmp_greater_equal(item, rows_.size()) || list_.column_subitem(column_index) != std::to_underlying(TrackColumn::Status)) {
        return {};
    }
    return row_status_tip(rows_[item]);
}

LRESULT TrackTable::on_custom_draw(const NMLVCUSTOMDRAW& draw) const {
    switch (draw.nmcd.dwDrawStage) {
    case CDDS_PREPAINT:      return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:  return CDRF_NOTIFYPOSTPAINT;
    case CDDS_ITEMPOSTPAINT: draw_status(draw.nmcd.hdc, static_cast<int>(draw.nmcd.dwItemSpec)); return CDRF_DODEFAULT;
    default:                 return CDRF_DODEFAULT;
    }
}

void TrackTable::draw_status(HDC dc, int item) const {
    std::optional<int> column = list_.column_index(std::to_underlying(TrackColumn::Status));
    if (!column || item < 0 || std::cmp_greater_equal(item, rows_.size())) {
        return;
    }
    const TrackRow& row = rows_[item];
    std::optional<StatusTab> status = row.duplicate_of ? std::nullopt : std::optional(row.status);
    auto found = std::ranges::find(status_icons_, status, &StatusIcon::status);
    if (found == status_icons_.end()) {
        return;
    }
    RECT cell = list_.cell_rect(item, *column);
    framework::Dpi dpi = framework::Dpi::of(list_.hwnd());
    const framework::IconResource& icon = found->icon;
    icon.draw(dc, cell.left + dpi.scale(StatusIconLeftDips), cell.top + (cell.bottom - cell.top - icon.size()) / 2);
}

}
