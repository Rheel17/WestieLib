#pragma once

#include <variant>

#include "core/playlists/PlaylistLookedUp.h"
#include "core/tools/ToolStatus.h"
#include "core/tools/ToolsChecked.h"
#include "core/tools/ToolsRunChanged.h"

namespace westie::core {

using CoreEvent = std::variant<ToolsRunChanged, ToolStatus, ToolsChecked, PlaylistLookedUp>;

}
