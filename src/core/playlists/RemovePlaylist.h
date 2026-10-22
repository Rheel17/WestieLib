#pragma once

#include <string>

#include "core/state/StateStore.h"

namespace westie::core {

bool remove_playlist(StateStore& store, const std::string& key);

}
