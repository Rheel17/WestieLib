#pragma once

#include <functional>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/UniqueHandle.h"
#include "ui/framework/Control.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/MessageFont.h"
#include "ui/framework/Tooltip.h"

namespace westie::ui::framework {

enum class SortDirection { Ascending, Descending };

struct ListColumn {
    const wchar_t* title = nullptr;
    int width = 0;
    // Stays with the column wherever it moves. The list asks for cell texts by the column's index, which
    // column_subitem turns back into this
    int subitem = 0;
    bool right_aligned = false;
};

struct ListSortArrow {
    int index = 0;
    SortDirection direction = SortDirection::Ascending;
};

class ListView final : public Control {
public:
    explicit ListView(HWND parent);

    void apply_dpi(Dpi dpi, const MessageFont& font, int row_height_dips);
    int column_count() const;
    void insert_column(int index, const ListColumn& column);
    void delete_column(int index);
    int column_subitem(int index) const;
    std::optional<int> column_index(int subitem) const;
    int column_width(int index) const;
    void set_column_width(int index, int width);
    std::vector<int> column_order() const;
    void set_column_order(std::span<const int> order);
    void set_sort_arrow(std::optional<ListSortArrow> arrow);
    void set_item_count(int count);
    void scroll_to_top();
    RECT cell_rect(int item, int column_index) const;
    std::vector<int> selected_items() const;
    std::optional<int> focused_item() const;
    void select_items(std::span<const int> items, std::optional<int> focused);
    void set_cell_tip(std::function<std::wstring(int item, int column_index)> cell_tip);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    HWND header() const;
    LRESULT on_header_draw(const NMCUSTOMDRAW& draw, LRESULT result) const;
    void click_column(int index) const;
    void show_cell_tip(LPARAM point);
    void forget_cell_tip();

    Dpi dpi_;
    platform::win32::UniqueImageList row_height_;
    std::function<std::wstring(int item, int column_index)> cell_tip_;
    Tooltip tooltip_;
    // The cell the tooltip is for, as an item and a column index
    std::optional<std::pair<int, int>> tip_cell_;
};

}
