#pragma once

#include <string>

#include "ui/TrackCounts.h"

namespace westie::ui {

std::wstring track_count_text(int count);
std::wstring synced_count_text(const TrackCounts& counts);

}
