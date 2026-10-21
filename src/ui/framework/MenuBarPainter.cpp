#include "ui/framework/MenuBarPainter.h"

#include <array>

#include <windows.h>
#include <uxtheme.h>
#include <vssym32.h>

#include "platform/win32/Win32Error.h"
#include "ui/framework/WindowDc.h"

namespace westie::ui::framework {

namespace {

// Undocumented messages Windows sends while it paints a window's menu bar; lparam points to the structs below. They
// let the window draw the bar itself, which is the only way to get rid of the gray row the theme draws under it.
constexpr UINT WmUahDrawMenu = 0x0091;
constexpr UINT WmUahDrawMenuItem = 0x0092;

struct UahMenu {
    HMENU menu = nullptr;
    HDC dc = nullptr;
    DWORD flags = 0;
};

// The start of the struct Windows passes; it goes on with metrics this code doesn't read
struct UahDrawMenuItem {
    DRAWITEMSTRUCT item{};
    UahMenu menu{};
    int position = 0;
};

int item_state(UINT state) noexcept {
    // An inactive window's menu bar has the disabled look
    bool disabled = state & (ODS_GRAYED | ODS_DISABLED | ODS_INACTIVE);
    if (state & ODS_SELECTED) {
        return disabled ? MBI_DISABLEDPUSHED : MBI_PUSHED;
    }
    if (state & ODS_HOTLIGHT) {
        return disabled ? MBI_DISABLEDHOT : MBI_HOT;
    }
    return disabled ? MBI_DISABLED : MBI_NORMAL;
}

}

MenuBarPainter::MenuBarPainter(HWND window, HBRUSH background) noexcept : window_(window), background_(background) {}

void MenuBarPainter::load_theme() {
    // Without visual styles there's no theme, and Windows draws the menu bar as usual
    theme_.reset(OpenThemeData(window_, L"Menu"));
}

bool MenuBarPainter::draw(UINT message, LPARAM lparam) const {
    if (!theme_) {
        return false;
    }
    if (message == WmUahDrawMenu) {
        draw_background(reinterpret_cast<const UahMenu*>(lparam)->dc);
        return true;
    }
    if (message == WmUahDrawMenuItem) {
        const auto& item = *reinterpret_cast<const UahDrawMenuItem*>(lparam);
        draw_item(item.menu.dc, item.menu.menu, item.position, item.item);
        return true;
    }
    return false;
}

void MenuBarPainter::cover_edge() const {
    if (!theme_ || !GetMenu(window_)) {
        return;
    }

    // Windows draws a gray line between the menu bar and the client area, outside the bar it lets us paint
    POINT client{0, 0};
    platform::win32::check(ClientToScreen(window_, &client), "ClientToScreen");
    RECT frame;
    platform::win32::check(GetWindowRect(window_, &frame), "GetWindowRect");
    RECT edge = bar_rect();
    edge.top = edge.bottom;
    edge.bottom = client.y - frame.top;
    WindowDc dc(window_);
    FillRect(dc.get(), &edge, background_);
}

RECT MenuBarPainter::bar_rect() const {
    MENUBARINFO bar{};
    bar.cbSize = sizeof(bar);
    platform::win32::check(GetMenuBarInfo(window_, OBJID_MENU, 0, &bar), "GetMenuBarInfo");
    RECT frame;
    platform::win32::check(GetWindowRect(window_, &frame), "GetWindowRect");
    RECT rect = bar.rcBar;
    OffsetRect(&rect, -frame.left, -frame.top);
    return rect;
}

void MenuBarPainter::draw_background(HDC dc) const {
    RECT bar = bar_rect();
    FillRect(dc, &bar, background_);
}

void MenuBarPainter::draw_item(HDC dc, HMENU menu, int position, const DRAWITEMSTRUCT& item) const {
    std::array<wchar_t, 256> text{};
    MENUITEMINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = MIIM_STRING;
    info.dwTypeData = text.data();
    info.cch = static_cast<UINT>(text.size() - 1);
    platform::win32::check(GetMenuItemInfoW(menu, static_cast<UINT>(position), TRUE, &info), "GetMenuItemInfoW");

    int state = item_state(item.itemState);
    FillRect(dc, &item.rcItem, background_);
    if (state != MBI_NORMAL && state != MBI_DISABLED) {
        platform::win32::check_hresult(DrawThemeBackground(theme_.get(), dc, MENU_BARITEM, state, &item.rcItem, nullptr), "DrawThemeBackground");
    }
    DWORD format = DT_CENTER | DT_VCENTER | DT_SINGLELINE | (item.itemState & ODS_NOACCEL ? DT_HIDEPREFIX : 0);
    platform::win32::check_hresult(
            DrawThemeText(theme_.get(), dc, MENU_BARITEM, state, text.data(), static_cast<int>(info.cch), format, 0, &item.rcItem), "DrawThemeText"
    );
}

}
