#pragma once

#include <optional>
#include <string>

#include "core/playlists/PlaylistLink.h"
#include "core/playlists/PlaylistLookedUp.h"
#include "core/playlists/PlaylistLookup.h"
#include "ui/dialogs/PlaylistLinkCheck.h"
#include "ui/tools/ToolBoard.h"

namespace westie::ui {

enum class LookupState { None, WaitingForTools, LookingUp, Found, Unavailable, Failed };

struct LookupStatus {
    LookupState state = LookupState::None;
    std::string name{};
    std::string error{};
};

class LinkLookup {
public:
    LinkLookup(core::PlaylistLookup& lookup, const ToolBoard& tools);

    bool set_link(const PlaylistLinkCheck& check);
    void start();
    bool tools_changed();
    bool apply(const core::PlaylistLookedUp& answer);
    void stop() noexcept;

    const LookupStatus& status() const noexcept;

private:
    bool tools_available() const noexcept;

    core::PlaylistLookup& lookup_;
    const ToolBoard& tools_;
    std::optional<core::PlaylistLink> link_;
    LookupStatus status_;
};

}
