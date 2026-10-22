#include "ui/dialogs/PlaylistLinkCheck.h"

#include <utility>

#include "platform/win32/Utf16.h"

namespace westie::ui {

namespace {

constexpr std::wstring_view Spaces = L" \t\r\n";

}

PlaylistLinkCheck check_playlist_link(std::wstring_view text, const AddedPlaylistName& added_name) {
    if (text.find_first_not_of(Spaces) == std::wstring_view::npos) {
        return {};
    }
    std::optional<core::PlaylistLink> link = core::parse_playlist_link(platform::win32::to_utf8(text));
    if (!link) {
        return {.state = LinkState::NotSupported};
    }
    if (std::optional<std::string> name = added_name(*link)) {
        return {.link = std::move(link), .state = LinkState::AlreadyAdded, .added_name = std::move(*name)};
    }
    return {.link = std::move(link), .state = LinkState::New};
}

}
