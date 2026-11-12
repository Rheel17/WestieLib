#pragma once

#include <string>

#include "core/log/Log.h"
#include "core/state/StateStore.h"

namespace westie::core {

bool remove_playlist(StateStore& store, Log& log, const std::string& key);

}
