#include "ui/tracks/TrackTable.h"

#include <array>
#include <optional>
#include <utility>
#include <vector>

namespace westie::ui {

namespace {

constexpr int RowHeightDips = 22;

struct ColumnSpec {
    TrackColumn column = TrackColumn::Title;
    const wchar_t* title = nullptr;
    int width_dips = 0;
    bool right_aligned = false;
    bool shown = true;
};

// A position only means something inside a playlist, so # shows only in a playlist scope, always first
constexpr ColumnSpec NumberColumn{.column = TrackColumn::Number, .title = L"#", .width_dips = 34, .right_aligned = true};

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

framework::ListColumn list_column(const ColumnSpec& spec, framework::Dpi dpi) {
    return {
        .title = spec.title,
        .width = dpi.scale(spec.width_dips),
        .subitem = std::to_underlying(spec.column),
        .right_aligned = spec.right_aligned,
    };
}

TrackSort default_sort(bool playlist) {
    return {.column = playlist ? TrackColumn::Number : TrackColumn::Title};
}

}

TrackTable::TrackTable(HWND parent) : list_(parent), sort_(default_sort(false)) {
    framework::Dpi dpi = framework::Dpi::of(list_.hwnd());
    std::vector<int> subitems;
    for (const ColumnSpec& spec : Columns) {
        if (spec.shown) {
            list_.insert_column(spec.column == TrackColumn::Title ? 0 : list_.column_count(), list_column(spec, dpi));
            subitems.push_back(std::to_underlying(spec.column));
        }
    }
    std::vector<int> order;
    for (int subitem : subitems) {
        order.push_back(*list_.column_index(subitem));
    }
    list_.set_column_order(order);
    show_sort();
}

HWND TrackTable::hwnd() const noexcept {
    return list_.hwnd();
}

void TrackTable::apply_dpi(framework::Dpi dpi, const framework::MessageFont& font) {
    list_.apply_dpi(dpi, font, RowHeightDips);
}

void TrackTable::show_scope(bool playlist) {
    std::optional<int> number = list_.column_index(std::to_underlying(TrackColumn::Number));
    if (playlist && !number) {
        add_number_column();
    } else if (!playlist && number) {
        list_.delete_column(*number);
    }
    sort_ = default_sort(playlist);
    show_sort();
}

LRESULT TrackTable::on_notify(NMHDR& header) {
    switch (header.code) {
    case LVN_COLUMNCLICK:  sort_by(reinterpret_cast<const NMLISTVIEW&>(header).iSubItem); return 0;
    case LVN_GETDISPINFOW: show_cell(reinterpret_cast<NMLVDISPINFOW&>(header).item); return 0;
    default:               return 0;
    }
}

void TrackTable::add_number_column() {
    int index = list_.column_count();
    list_.insert_column(index, list_column(NumberColumn, framework::Dpi::of(list_.hwnd())));
    std::vector<int> order = list_.column_order();
    std::erase(order, index);
    order.insert(order.begin(), index);
    list_.set_column_order(order);
}

void TrackTable::sort_by(int column_index) {
    auto column = static_cast<TrackColumn>(list_.column_subitem(column_index));
    if (column == sort_.column) {
        bool ascending = sort_.direction == framework::SortDirection::Ascending;
        sort_.direction = ascending ? framework::SortDirection::Descending : framework::SortDirection::Ascending;
    } else {
        sort_ = {.column = column};
    }
    show_sort();
}

void TrackTable::show_sort() {
    std::optional<int> index = list_.column_index(std::to_underlying(sort_.column));
    list_.set_sort_arrow(index ? std::optional(framework::ListSortArrow{.index = *index, .direction = sort_.direction}) : std::nullopt);
}

void TrackTable::show_cell(LVITEMW& item) {
    // Until there are tracks, every cell is empty
    if ((item.mask & LVIF_TEXT) && item.cchTextMax > 0) {
        item.pszText[0] = L'\0';
    }
}

}
