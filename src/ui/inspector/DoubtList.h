#pragma once

#include <optional>
#include <string>

#include "ui/inspector/TrackDetails.h"

namespace westie::ui {

std::wstring doubt_list(const std::optional<TrackMatch>& match);

}
