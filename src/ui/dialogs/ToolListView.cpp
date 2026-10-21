#include "ui/dialogs/ToolListView.h"

#include <array>
#include <utility>

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#include "ResourceIds.h"
#include "platform/win32/Utf16.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/Dpi.h"
#include "ui/tools/ToolMark.h"
#include "ui/tools/ToolStatusText.h"

namespace westie::ui {

namespace {

constexpr int IconDips = 12;
constexpr int ToolWidthDips = 100;
constexpr int ReleaseWidthDips = 110;

// In the order of ToolMark
constexpr std::array MarkIcons = {IDI_SYNCED, IDI_SYNCING, IDI_WARN};

// The status is column 0, shown last: only column 0 has the item's icon, and the other columns then leave no room
// for one
enum class Column { Status, Tool, Release };

}

ToolListView::ToolListView(HWND list) : list_(list), icons_(MarkIcons, framework::Dpi::of(list).scale(IconDips)) {
    platform::win32::check_hresult(SetWindowTheme(list_, L"Explorer", nullptr), "SetWindowTheme");
    ListView_SetExtendedListViewStyle(list_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    ListView_SetImageList(list_, icons_.handle(), LVSIL_SMALL);
    add_columns();
}

void ToolListView::show(const ToolBoard& board) {
    auto statuses = board.statuses();
    for (int item = 0; std::cmp_less(item, statuses.size()); ++item) {
        const core::ToolStatus& status = statuses[item];
        std::wstring state = tool_state_text(status);
        LVITEMW row{};
        row.mask = LVIF_TEXT | LVIF_IMAGE;
        row.iItem = item;
        row.iImage = std::to_underlying(tool_mark(status));
        row.pszText = state.data();
        if (item >= ListView_GetItemCount(list_)) {
            if (ListView_InsertItem(list_, &row) == -1) {
                platform::win32::throw_last_error("LVM_INSERTITEMW");
            }
        } else {
            platform::win32::check(ListView_SetItem(list_, &row), "LVM_SETITEMW");
        }
        set_text(item, std::to_underlying(Column::Tool), tool_name_text(status.package));
        set_text(item, std::to_underlying(Column::Release), platform::win32::to_utf16(status.release));
    }
}

void ToolListView::add_columns() {
    framework::Dpi dpi = framework::Dpi::of(list_);
    RECT client;
    platform::win32::check(GetClientRect(list_, &client), "GetClientRect");
    int tool_width = dpi.scale(ToolWidthDips);
    int release_width = dpi.scale(ReleaseWidthDips);

    struct ColumnSpec {
        const wchar_t* title = nullptr;
        int width = 0;
    };
    std::array<ColumnSpec, 3> columns = {{
        {.title = L"Status", .width = client.right - tool_width - release_width},
        {.title = L"Tool", .width = tool_width},
        {.title = L"Release", .width = release_width},
    }};
    for (int index = 0; std::cmp_less(index, columns.size()); ++index) {
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.pszText = const_cast<wchar_t*>(columns[index].title);
        column.cx = columns[index].width;
        if (ListView_InsertColumn(list_, index, &column) == -1) {
            platform::win32::throw_last_error("LVM_INSERTCOLUMNW");
        }
    }

    std::array order = {std::to_underlying(Column::Tool), std::to_underlying(Column::Release), std::to_underlying(Column::Status)};
    platform::win32::check(ListView_SetColumnOrderArray(list_, static_cast<int>(order.size()), order.data()), "LVM_SETCOLUMNORDERARRAY");
}

void ToolListView::set_text(int item, int column, const std::wstring& text) {
    LVITEMW cell{};
    cell.iSubItem = column;
    cell.pszText = const_cast<wchar_t*>(text.c_str());
    SendMessageW(list_, LVM_SETITEMTEXTW, item, reinterpret_cast<LPARAM>(&cell));
}

}
