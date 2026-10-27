#pragma once

#include <string>

#include "ui/tracks/TrackRows.h"

namespace westie::ui {

std::wstring row_status_word(const TrackRow& row);
std::wstring row_status_tip(const TrackRow& row);
std::wstring duplicate_explanation(const RowOriginal& original);

}
