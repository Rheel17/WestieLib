#include "ui/dialogs/PlayButton.h"

#include <utility>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/Rect.h"

namespace westie::ui {

namespace {

constexpr int IconDips = 12;
constexpr UINT_PTR TipTool = 1;

}

PlayButton::PlayButton(HWND button, const wchar_t* tip) : button_(button), tooltip_(button) {
    // The button only moves, so its client area stays the tool's
    RECT client;
    platform::win32::check(GetClientRect(button_, &client), "GetClientRect");
    tooltip_.set_tool(TipTool, framework::Rect::from_rect(client), tip);
    apply_dpi();
}

void PlayButton::set_enabled(bool enabled) {
    EnableWindow(button_, enabled);
    show_icon(*icon_, *disabled_icon_);
}

void PlayButton::apply_dpi() {
    // The button keeps drawing the old icon until it gets the new one
    int size = framework::Dpi::of(button_).scale(IconDips);
    framework::IconResource icon(IDI_PLAY, size);
    framework::IconResource disabled_icon(IDI_PLAY_DISABLED, size);
    show_icon(icon, disabled_icon);
    icon_.emplace(std::move(icon));
    disabled_icon_.emplace(std::move(disabled_icon));
}

void PlayButton::show_icon(const framework::IconResource& icon, const framework::IconResource& disabled_icon) const {
    HICON shown = IsWindowEnabled(button_) ? icon.handle() : disabled_icon.handle();
    SendMessageW(button_, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(shown));
}

}
