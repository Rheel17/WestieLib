#include "ui/toolbar/MainToolbar.h"

#include <algorithm>
#include <array>
#include <utility>

#include "ResourceIds.h"
#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"
#include "ui/commands/CommandMenu.h"
#include "ui/framework/PaintScope.h"
#include "ui/framework/PopupMenu.h"
#include "ui/framework/Rect.h"
#include "ui/framework/Size.h"
#include "ui/framework/WindowClass.h"

namespace westie::ui {

namespace {

constexpr int HeightDips = 34;
constexpr int ButtonHeightDips = 26;
constexpr int ButtonSidePaddingDips = 8;
constexpr int IconDips = 16;
constexpr int IconGapDips = 5;
constexpr int ButtonSpacingDips = 1;
constexpr int SeparatorWidthDips = 11;
constexpr int SeparatorHeightDips = 20;
constexpr int LeftPaddingDips = 4;
// A hairline stays one pixel at every scale
constexpr int HairlinePixels = 1;
constexpr COLORREF RuleColor = RGB(0xD9, 0xD9, 0xD9);
constexpr COLORREF SeparatorColor = RGB(0xD5, 0xD5, 0xD5);
constexpr COLORREF DisabledTextColor = RGB(0xA0, 0xA0, 0xA0);

constexpr std::array Glyphs = {IDI_PLUS, IDI_SYNCING, IDI_STOP, IDI_GEAR};
constexpr std::array DisabledGlyphs = {IDI_PLUS_DISABLED, IDI_SYNCING_DISABLED, IDI_STOP_DISABLED, IDI_GEAR_DISABLED};

constexpr std::array Buttons = {
    framework::ToolbarButton{.command = std::to_underlying(CommandId::AddPlaylist), .image = 0, .text = L"Add playlist"},
    framework::ToolbarButton{.command = std::to_underlying(CommandId::SyncAll), .image = 1, .text = L"Sync all", .dropdown = true},
    framework::ToolbarButton{.command = std::to_underlying(CommandId::CancelSync), .image = 2, .text = L"Cancel"},
    framework::ToolbarSeparator,
    framework::ToolbarButton{.command = std::to_underlying(CommandId::Settings), .image = 3, .text = L"Settings"},
};

constexpr std::array SyncMenuItems = {
    command_item(CommandId::SyncSelectedPlaylist, L"Sync &selected playlist"),
};

}

MainToolbar::MainToolbar(HWND parent, std::function<void(CommandId)> on_command) :
        Window(window_class(), {.style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, .parent = parent}),
        on_command_(std::move(on_command)),
        toolbar_(hwnd()) {
    toolbar_.add_buttons(Buttons);
}

int MainToolbar::height() const noexcept {
    return height_;
}

void MainToolbar::apply_dpi(framework::Dpi dpi, const framework::MessageFont& font) {
    int icon_size = dpi.scale(IconDips);
    framework::IconImageList images(Glyphs, icon_size);
    framework::IconImageList disabled_images(DisabledGlyphs, icon_size);
    toolbar_.set_image_lists(images.handle(), disabled_images.handle());
    toolbar_.apply_metrics(
            {
                .button_height = dpi.scale(ButtonHeightDips),
                .side_padding = dpi.scale(ButtonSidePaddingDips),
                .icon_gap = dpi.scale(IconGapDips),
                .button_spacing = dpi.scale(ButtonSpacingDips),
                .separator_width = dpi.scale(SeparatorWidthDips),
            },
            font.handle()
    );
    images_ = std::move(images);
    disabled_images_ = std::move(disabled_images);

    height_ = std::max(dpi.scale(HeightDips), toolbar_.button_size().height + dpi.scale(HeightDips - ButtonHeightDips));
    layout();
}

void MainToolbar::update(const CommandState& state) {
    state_ = state;
    for (const framework::ToolbarButton& button : Buttons) {
        if (button.text) {
            toolbar_.set_enabled(button.command, state.enabled(static_cast<CommandId>(button.command)));
        }
    }
}

LRESULT MainToolbar::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:    layout(); return 0;
    case WM_PAINT:   paint(); return 0;
    case WM_COMMAND: on_command_(static_cast<CommandId>(LOWORD(wparam))); return 0;
    case WM_NOTIFY:  return on_notify(*reinterpret_cast<const NMHDR*>(lparam));
    default:         return Window::handle_message(message, wparam, lparam);
    }
}

const framework::WindowClass& MainToolbar::window_class() {
    static framework::WindowClass window_class({.name = L"WestieLib.MainToolbar", .background = GetSysColorBrush(COLOR_WINDOW)});
    return window_class;
}

void MainToolbar::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    int left = framework::Dpi::of(hwnd()).scale(LeftPaddingDips);
    int width = std::max(0, static_cast<int>(client.right) - 2 * left);
    framework::Size button = toolbar_.button_size();
    int top = (client.bottom - HairlinePixels - button.height) / 2;
    platform::win32::check(SetWindowPos(toolbar_.hwnd(), nullptr, left, top, width, button.height, SWP_NOZORDER | SWP_NOACTIVATE), "SetWindowPos");
}

void MainToolbar::paint() {
    framework::PaintScope paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    RECT rule{client.left, client.bottom - HairlinePixels, client.right, client.bottom};
    SetDCBrushColor(paint.dc(), RuleColor);
    FillRect(paint.dc(), &rule, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
}

LRESULT MainToolbar::on_notify(const NMHDR& header) {
    switch (header.code) {
    case NM_CUSTOMDRAW: return on_custom_draw(reinterpret_cast<const NMCUSTOMDRAW&>(header));
    case TBN_DROPDOWN:  show_sync_menu(reinterpret_cast<const NMTOOLBARW&>(header)); return TBDDRET_DEFAULT;
    default:            return 0;
    }
}

LRESULT MainToolbar::on_custom_draw(const NMCUSTOMDRAW& draw) const {
    if (draw.dwDrawStage == CDDS_PREPAINT) {
        // Otherwise the toolbar keeps its own gray background
        FillRect(draw.hdc, &draw.rc, GetSysColorBrush(COLOR_WINDOW));
        return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
    }
    if (draw.dwDrawStage == CDDS_ITEMPREPAINT) {
        return draw.uItemState & CDIS_DISABLED ? CDRF_NOTIFYPOSTPAINT : CDRF_DODEFAULT;
    }
    if (draw.dwDrawStage == CDDS_ITEMPOSTPAINT) {
        // The toolbar draws a disabled button's text without ClearType, and no custom draw color reaches it
        toolbar_.redraw_text(draw, DisabledTextColor, GetSysColorBrush(COLOR_WINDOW));
    }
    if (draw.dwDrawStage == CDDS_POSTPAINT) {
        paint_separators(draw.hdc);
    }
    return CDRF_DODEFAULT;
}

void MainToolbar::paint_separators(HDC dc) const {
    // The theme's separator is dark, taller and off-center, so paint over it
    int height = framework::Dpi::of(hwnd()).scale(SeparatorHeightDips);
    SetDCBrushColor(dc, SeparatorColor);
    for (const framework::Rect& separator : toolbar_.separator_rects()) {
        RECT cover = separator.to_rect();
        FillRect(dc, &cover, GetSysColorBrush(COLOR_WINDOW));
        int left = separator.left + (separator.width() - HairlinePixels) / 2;
        int top = separator.top + (separator.height() - height) / 2;
        RECT line{left, top, left + HairlinePixels, top + height};
        FillRect(dc, &line, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    }
}

void MainToolbar::show_sync_menu(const NMTOOLBARW& notification) {
    platform::win32::UniqueMenu menu = framework::build_popup_menu(SyncMenuItems);
    update_menu_items(menu.get(), state_);

    RECT button = notification.rcButton;
    MapWindowPoints(toolbar_.hwnd(), nullptr, reinterpret_cast<POINT*>(&button), 2);
    TPMPARAMS exclude{.cbSize = sizeof(TPMPARAMS), .rcExclude = button};
    UINT flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_VERTICAL | TPM_RETURNCMD;
    auto command = static_cast<UINT>(TrackPopupMenuEx(menu.get(), flags, button.left, button.bottom, hwnd(), &exclude));
    if (command != 0) {
        on_command_(static_cast<CommandId>(command));
    }
}

}
