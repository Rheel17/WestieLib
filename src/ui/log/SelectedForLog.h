#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/state/State.h"
#include "ui/log/LogFilter.h"
#include "ui/tracks/TrackRows.h"

namespace westie::ui {

LogSelection selected_for_log(const core::State& state, const std::optional<std::string>& playlist, const std::vector<TrackRow>& selected_rows);

}
