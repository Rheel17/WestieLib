#pragma once

#include <chrono>
#include <string>

#include "core/sources/PlaylistContents.h"
#include "core/state/State.h"

namespace westie::core {

void apply_fetch(State& state, const std::string& playlist_key, PlaylistContents contents, std::chrono::system_clock::time_point now);
void apply_fetch_failure(State& state, const std::string& playlist_key, std::string error, std::chrono::system_clock::time_point now);

}
