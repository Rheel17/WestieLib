#include "ui/dialogs/PlaylistLinkCheck.h"

#include <format>
#include <utility>

#include "platform/win32/Utf16.h"
#include "ui/PlaylistTypeText.h"

namespace westie::ui {

namespace {

constexpr std::wstring_view Spaces = L" \t\r\n";

}

PlaylistLinkCheck check_playlist_link(std::wstring_view text, const std::function<bool(const core::PlaylistLink&)>& is_added) {
    if (text.find_first_not_of(Spaces) == std::wstring_view::npos) {
        return {};
    }
    std::optional<core::PlaylistLink> link = core::parse_playlist_link(platform::win32::to_utf8(text));
    if (!link) {
        return {.state = LinkState::NotSupported};
    }
    LinkState state = is_added(*link) ? LinkState::AlreadyAdded : LinkState::New;
    return {.link = std::move(link), .state = state};
}

std::wstring link_check_text(const PlaylistLinkCheck& check) {
    switch (check.state) {
    case LinkState::NotSupported: return L"Not a supported playlist link";
    case LinkState::AlreadyAdded: return std::format(L"{} · already added", playlist_type_text(check.link->type));
    case LinkState::New:          return playlist_type_text(check.link->type);
    default:                      return {};
    }
}

}
