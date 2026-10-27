#pragma once

#include "core/state/Track.h"
#include "ui/tracks/StatusCounts.h"

namespace westie::ui {

int status_icon(StatusTab status);
const wchar_t* status_explanation(StatusTab status, core::TrackFailure failure);
const wchar_t* failure_reason(core::TrackFailure failure);

}
