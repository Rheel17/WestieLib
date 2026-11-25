#pragma once

#include <map>
#include <string>

#include "core/library/ApplyRemovals.h"
#include "core/library/Library.h"
#include "core/log/Log.h"

namespace westie::core {

void resolve_removals(Library& library, Log& log, const std::map<std::string, RemovalDecision>& decisions);

}