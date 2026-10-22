#include "ui/dialogs/LinkLine.h"

#include <format>
#include <utility>

#include "ResourceIds.h"
#include "platform/win32/Utf16.h"
#include "ui/PlaylistTypeText.h"
#include "ui/SourceLogo.h"

namespace westie::ui {

std::wstring LinkLine::accessible_text() const {
    return source.empty() ? text : std::format(L"{} {}", text, source);
}

LinkLine link_line(const PlaylistLinkCheck& check, const LookupStatus& lookup) {
    if (check.state == LinkState::NotSupported) {
        return {.text = L"Not a supported playlist link", .gray = true};
    }
    if (!check.link) {
        return {};
    }

    int logo = source_logo_icon(check.link->type);
    std::wstring source = playlist_type_text(check.link->type);
    switch (lookup.state) {
    case LookupState::WaitingForTools: return {.icon = IDI_TOOL_WAIT, .text = L"Waiting for required tools to be installed…", .gray = true};
    case LookupState::LookingUp:       return {.spinner = true, .text = std::format(L"Looking up the {}…", source), .gray = true};
    case LookupState::Found:           return {.icon = logo, .text = platform::win32::to_utf16(lookup.name), .source = std::move(source)};
    case LookupState::Unavailable:     return {.icon = IDI_WARN, .text = L"This playlist doesn't exist or is private."};
    case LookupState::Failed:          return {.icon = IDI_WARN, .text = std::format(L"Couldn't look it up: {}", platform::win32::to_utf16(lookup.error))};
    case LookupState::None:            return {};
    }
    return {};
}

std::wstring add_button_text(const PlaylistLinkCheck& check) {
    return check.state == LinkState::AlreadyAdded ? L"Already added" : L"Add";
}

}
