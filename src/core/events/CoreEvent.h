#pragma once

#include <variant>

#include "core/playlists/PlaylistLookedUp.h"
#include "core/sync/PlaylistFetched.h"
#include "core/sync/PlaylistFetchFailed.h"
#include "core/sync/PlaylistFetching.h"
#include "core/sync/PlaylistPageFetched.h"
#include "core/sync/SyncFinished.h"
#include "core/sync/SyncQueued.h"
#include "core/tools/ToolStatus.h"
#include "core/tools/ToolsChecked.h"
#include "core/tools/ToolsRunChanged.h"

namespace westie::core {

using CoreEvent = std::variant<
        ToolsRunChanged, ToolStatus, ToolsChecked, PlaylistLookedUp, SyncQueued, PlaylistFetching, PlaylistPageFetched, PlaylistFetched, PlaylistFetchFailed,
        SyncFinished>;

}
