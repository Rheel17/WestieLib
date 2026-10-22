#pragma once

#include <windows.h>

#include "ui/TrackCounts.h"
#include "ui/framework/Rect.h"

namespace westie::ui {

void paint_sync_bar(HDC dc, framework::Rect bar, const TrackCounts& counts);

}
