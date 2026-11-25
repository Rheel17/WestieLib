#pragma once

#include <string>

#include "core/library/Library.h"
#include "core/log/Log.h"

namespace westie::core {

bool remove_playlist(Library& library, Log& log, const std::string& key);

}
