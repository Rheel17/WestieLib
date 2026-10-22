#include "ui/framework/Button.h"

#include <algorithm>
#include <array>
#include <utility>

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <vssym32.h>

#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

namespace {

constexpr int MinWidthDips = 75;
constexpr int MinHeightDips = 23;
constexpr int SidePaddingDips = 10;
constexpr int CompactSidePaddingDips = 9;
constexpr int TextHeightPaddingDips = 7;
constexpr int IconDips = 16;
constexpr int IconGapDips = 5;

}

Button::Button(HWND parent, const Options& options) :
        Control({
            .class_name = WC_BUTTONW,
            .text = options.text,
            .style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | static_cast<DWORD>(options.default_button ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON) |
                     static_cast<DWORD>(options.icon != 0 ? BS_LEFT : 0),
            .parent = parent,
            .id = options.command,
        }),
        text_(options.text),
        icon_resource_(options.icon),
        compact_(options.compact) {}

void Button::apply_dpi(Dpi dpi, const MessageFont& font) {
    set_font(font.handle());
    int content = text_width(font.handle()) + (icon_resource_ != 0 ? dpi.scale(IconDips) + dpi.scale(IconGapDips) : 0);
    int min_width = compact_ ? 0 : dpi.scale(MinWidthDips);
    int side_padding = dpi.scale(compact_ ? CompactSidePaddingDips : SidePaddingDips);
    size_ = {
        std::max(min_width, content + 2 * side_padding),
        std::max(dpi.scale(MinHeightDips), font.line_height() + dpi.scale(TextHeightPaddingDips)),
    };
    if (icon_resource_ != 0) {
        set_icon(dpi, size_.width - content);
    }
}

Size Button::size() const noexcept {
    return size_;
}

void Button::set_enabled(bool enabled) {
    EnableWindow(hwnd(), enabled ? TRUE : FALSE);
}

void Button::set_icon(Dpi dpi, int spare_width) {
    // The button starts the icon inside its theme's content margin, then the text after the icon's right margin
    int left = spare_width / 2 - content_left_margin();

    // The button keeps drawing from the old list until it has the new one
    IconImageList icon(std::array{icon_resource_}, dpi.scale(IconDips));
    BUTTON_IMAGELIST images{.himl = icon.handle(), .margin = {left, 0, dpi.scale(IconGapDips), 0}, .uAlign = BUTTON_IMAGELIST_ALIGN_LEFT};
    platform::win32::check(SendMessageW(hwnd(), BCM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(&images)), "BCM_SETIMAGELIST");
    RECT no_margin{};
    platform::win32::check(SendMessageW(hwnd(), BCM_SETTEXTMARGIN, 0, reinterpret_cast<LPARAM>(&no_margin)), "BCM_SETTEXTMARGIN");
    icon_ = std::move(icon);
}

int Button::content_left_margin() const {
    // Without visual styles there is no theme, and no margin to allow for
    platform::win32::UniqueTheme theme(OpenThemeData(hwnd(), L"Button"));
    if (!theme) {
        return 0;
    }
    MARGINS margins{};
    platform::win32::check_hresult(GetThemeMargins(theme.get(), nullptr, BP_PUSHBUTTON, PBS_NORMAL, TMT_CONTENTMARGINS, nullptr, &margins), "GetThemeMargins");
    return margins.cxLeftWidth;
}

int Button::text_width(HFONT font) const {
    platform::win32::UniqueMemoryDc dc(platform::win32::check(CreateCompatibleDC(nullptr), "CreateCompatibleDC"));
    SelectObject(dc.get(), font);
    SIZE extent{};
    platform::win32::check(GetTextExtentPoint32W(dc.get(), text_.c_str(), static_cast<int>(text_.size()), &extent), "GetTextExtentPoint32W");
    return extent.cx;
}

}
