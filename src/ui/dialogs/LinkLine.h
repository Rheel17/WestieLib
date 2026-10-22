#pragma once

#include <string>

#include "ui/dialogs/LinkLookup.h"
#include "ui/dialogs/PlaylistLinkCheck.h"

namespace westie::ui {

struct LinkLine {
    // An icon resource, or 0 for none
    int icon = 0;
    // Turns where the icon would be, while something goes on
    bool spinner = false;
    std::wstring text{};
    bool gray = false;
    // Follows the text, in gray
    std::wstring source{};

    std::wstring accessible_text() const;
};

LinkLine link_line(const PlaylistLinkCheck& check, const LookupStatus& lookup);
std::wstring add_button_text(const PlaylistLinkCheck& check);

}
