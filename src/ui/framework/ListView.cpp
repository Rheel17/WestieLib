#include "ui/framework/ListView.h"

#include <utility>

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

namespace {

// A hairline stays one pixel at every scale
constexpr int HairlinePixels = 1;
constexpr COLORREF RuleColor = RGB(0xE5, 0xE5, 0xE5);

}

ListView::ListView(HWND parent) :
        Control({
            .class_name = WC_LISTVIEWW,
            // The list keeps its image list for the row height, so it mustn't destroy it
            .style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS,
            .parent = parent,
        }),
        dpi_(Dpi::of(hwnd())) {
    platform::win32::check_hresult(SetWindowTheme(hwnd(), L"Explorer", nullptr), "SetWindowTheme");
    DWORD styles = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP | LVS_EX_INFOTIP;
    SendMessageW(hwnd(), LVM_SETEXTENDEDLISTVIEWSTYLE, styles, styles);
}

void ListView::apply_dpi(Dpi dpi, const MessageFont& font, int row_height_dips) {
    set_font(font.handle());

    // A report view's rows are a pixel taller than its small images, when those are taller than the text
    int image_height = dpi.scale(row_height_dips) - 1;
    platform::win32::UniqueImageList row_height(platform::win32::check(ImageList_Create(1, image_height, ILC_COLOR32, 0, 0), "ImageList_Create"));
    SendMessageW(hwnd(), LVM_SETIMAGELIST, LVSIL_SMALL, reinterpret_cast<LPARAM>(row_height.get()));
    row_height_ = std::move(row_height);

    if (dpi.value() != dpi_.value()) {
        for (int index = 0; index < column_count(); ++index) {
            auto width = static_cast<int>(SendMessageW(hwnd(), LVM_GETCOLUMNWIDTH, index, 0));
            int scaled = MulDiv(width, static_cast<int>(dpi.value()), static_cast<int>(dpi_.value()));
            platform::win32::check(SendMessageW(hwnd(), LVM_SETCOLUMNWIDTH, index, MAKELPARAM(scaled, 0)), "LVM_SETCOLUMNWIDTH");
        }
        dpi_ = dpi;
    }
}

int ListView::column_count() const {
    return static_cast<int>(SendMessageW(header(), HDM_GETITEMCOUNT, 0, 0));
}

void ListView::insert_column(int index, const ListColumn& column) {
    LVCOLUMNW entry{};
    entry.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
    entry.fmt = column.right_aligned ? LVCFMT_RIGHT : LVCFMT_LEFT;
    // The list copies the text and never writes to it
    entry.pszText = const_cast<wchar_t*>(column.title);
    entry.cx = column.width;
    entry.iSubItem = column.subitem;
    if (SendMessageW(hwnd(), LVM_INSERTCOLUMNW, index, reinterpret_cast<LPARAM>(&entry)) == -1) {
        platform::win32::throw_last_error("LVM_INSERTCOLUMNW");
    }
}

void ListView::delete_column(int index) {
    platform::win32::check(SendMessageW(hwnd(), LVM_DELETECOLUMN, index, 0), "LVM_DELETECOLUMN");
}

int ListView::column_subitem(int index) const {
    LVCOLUMNW entry{};
    entry.mask = LVCF_SUBITEM;
    platform::win32::check(SendMessageW(hwnd(), LVM_GETCOLUMNW, index, reinterpret_cast<LPARAM>(&entry)), "LVM_GETCOLUMNW");
    return entry.iSubItem;
}

std::optional<int> ListView::column_index(int subitem) const {
    for (int index = 0; index < column_count(); ++index) {
        if (column_subitem(index) == subitem) {
            return index;
        }
    }
    return std::nullopt;
}

std::vector<int> ListView::column_order() const {
    std::vector<int> order(column_count());
    platform::win32::check(SendMessageW(hwnd(), LVM_GETCOLUMNORDERARRAY, order.size(), reinterpret_cast<LPARAM>(order.data())), "LVM_GETCOLUMNORDERARRAY");
    return order;
}

void ListView::set_column_order(std::span<const int> order) {
    platform::win32::check(SendMessageW(hwnd(), LVM_SETCOLUMNORDERARRAY, order.size(), reinterpret_cast<LPARAM>(order.data())), "LVM_SETCOLUMNORDERARRAY");
}

void ListView::set_sort_arrow(std::optional<ListSortArrow> arrow) {
    for (int index = 0; index < column_count(); ++index) {
        HDITEMW item{};
        item.mask = HDI_FORMAT;
        platform::win32::check(SendMessageW(header(), HDM_GETITEMW, index, reinterpret_cast<LPARAM>(&item)), "HDM_GETITEMW");
        item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
        if (arrow && arrow->index == index) {
            item.fmt |= arrow->direction == SortDirection::Ascending ? HDF_SORTUP : HDF_SORTDOWN;
        }
        platform::win32::check(SendMessageW(header(), HDM_SETITEMW, index, reinterpret_cast<LPARAM>(&item)), "HDM_SETITEMW");
    }
}

void ListView::set_item_count(int count) {
    // Without NOSCROLL a new count scrolls the list back to the top
    platform::win32::check(SendMessageW(hwnd(), LVM_SETITEMCOUNT, count, LVSICF_NOSCROLL), "LVM_SETITEMCOUNT");
}

void ListView::scroll_to_top() {
    if (SendMessageW(hwnd(), LVM_GETITEMCOUNT, 0, 0) > 0) {
        SendMessageW(hwnd(), LVM_ENSUREVISIBLE, 0, FALSE);
    }
}

RECT ListView::cell_rect(int item, int column_index) const {
    // The message reads which rect from left, and which column from top
    RECT rect{LVIR_BOUNDS, column_index, 0, 0};
    platform::win32::check(SendMessageW(hwnd(), LVM_GETSUBITEMRECT, item, reinterpret_cast<LPARAM>(&rect)), "LVM_GETSUBITEMRECT");
    return rect;
}

std::vector<int> ListView::selected_items() const {
    std::vector<int> items;
    for (auto item = static_cast<int>(SendMessageW(hwnd(), LVM_GETNEXTITEM, -1, LVNI_SELECTED)); item != -1;
         item = static_cast<int>(SendMessageW(hwnd(), LVM_GETNEXTITEM, item, LVNI_SELECTED))) {
        items.push_back(item);
    }
    return items;
}

std::optional<int> ListView::focused_item() const {
    auto item = static_cast<int>(SendMessageW(hwnd(), LVM_GETNEXTITEM, -1, LVNI_FOCUSED));
    return item == -1 ? std::nullopt : std::optional(item);
}

void ListView::select_items(std::span<const int> items, std::optional<int> focused) {
    auto set_state = [this](int item, UINT state, UINT mask) {
        LVITEMW entry{};
        entry.state = state;
        entry.stateMask = mask;
        platform::win32::check(SendMessageW(hwnd(), LVM_SETITEMSTATE, item, reinterpret_cast<LPARAM>(&entry)), "LVM_SETITEMSTATE");
    };
    // Item -1 is every item
    set_state(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    for (int item : items) {
        set_state(item, LVIS_SELECTED, LVIS_SELECTED);
    }
    if (focused) {
        set_state(*focused, LVIS_FOCUSED, LVIS_FOCUSED);
    }
}

LRESULT ListView::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = Control::handle_message(message, wparam, lparam);
    if (message != WM_NOTIFY || reinterpret_cast<const NMHDR*>(lparam)->hwndFrom != header()) {
        return result;
    }
    switch (reinterpret_cast<const NMHDR*>(lparam)->code) {
    // The header reports a second quick click as a double click, which the list doesn't pass on as a column click
    case HDN_ITEMDBLCLICKW: click_column(reinterpret_cast<const NMHEADERW*>(lparam)->iItem); return result;
    case NM_CUSTOMDRAW:     return on_header_draw(*reinterpret_cast<const NMCUSTOMDRAW*>(lparam), result);
    default:                return result;
    }
}

LRESULT ListView::on_header_draw(const NMCUSTOMDRAW& draw, LRESULT result) const {
    if (draw.dwDrawStage == CDDS_PREPAINT) {
        return result | CDRF_NOTIFYPOSTPAINT;
    }
    if (draw.dwDrawStage == CDDS_POSTPAINT) {
        // The theme separates the titles only from each other, so a rule separates them from the rows
        RECT rule{draw.rc.left, draw.rc.bottom - HairlinePixels, draw.rc.right, draw.rc.bottom};
        SetDCBrushColor(draw.hdc, RuleColor);
        FillRect(draw.hdc, &rule, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    }
    return result;
}

HWND ListView::header() const {
    return reinterpret_cast<HWND>(SendMessageW(hwnd(), LVM_GETHEADER, 0, 0));
}

void ListView::click_column(int index) const {
    NMLISTVIEW click{};
    click.hdr.hwndFrom = hwnd();
    click.hdr.idFrom = static_cast<UINT_PTR>(GetDlgCtrlID(hwnd()));
    click.hdr.code = LVN_COLUMNCLICK;
    click.iItem = -1;
    click.iSubItem = index;
    SendMessageW(GetParent(hwnd()), WM_NOTIFY, click.hdr.idFrom, reinterpret_cast<LPARAM>(&click));
}

}
