#pragma once

#include <windows.h>

#include "ui/TrackCounts.h"
#include "ui/framework/Rect.h"

namespace westie::ui {

// Paused is a paused progress bar's yellow
enum class BarFill { Progress, Paused };

void paint_sync_bar(HDC dc, framework::Rect bar, const TrackCounts& counts);
// The same bar, filled as far as a fraction from 0 to 1
void paint_fraction_bar(HDC dc, framework::Rect bar, double fraction, BarFill fill = BarFill::Progress);

}
