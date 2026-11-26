#include "ui/log/LogMenu.h"

#include <array>
#include <utility>

#include "platform/win32/UniqueHandle.h"
#include "ui/framework/PopupMenu.h"

namespace westie::ui {

namespace {

constexpr framework::MenuItem action_item(LogLineAction action, const wchar_t* text) {
    return {.command = std::to_underlying(action), .text = text};
}

constexpr std::array LogLineItems = {
    action_item(LogLineAction::GoToTrack, L"&Go to track"),
    action_item(LogLineAction::GoToPlaylist, L"Go to &playlist"),
    framework::MenuSeparator,
    action_item(LogLineAction::Copy, L"&Copy\tCtrl+C"),
};

void enable(HMENU menu, LogLineAction action, bool enabled) {
    EnableMenuItem(menu, std::to_underlying(action), MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED));
}

}

std::optional<LogLineAction> choose_log_line_action(HWND owner, POINT screen, const LogLineMenu& menu) {
    platform::win32::UniqueMenu popup = framework::build_popup_menu(LogLineItems);
    enable(popup.get(), LogLineAction::GoToTrack, menu.track);
    enable(popup.get(), LogLineAction::GoToPlaylist, menu.playlist);
    std::optional<WORD> chosen = framework::choose_from_popup(owner, screen, popup.get());
    return chosen ? std::optional(static_cast<LogLineAction>(*chosen)) : std::nullopt;
}

}
