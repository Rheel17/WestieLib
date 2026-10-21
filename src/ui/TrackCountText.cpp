#include "ui/TrackCountText.h"

#include <format>

namespace westie::ui {

std::wstring track_count_text(int count) {
    return std::format(L"{} {}", count, count == 1 ? L"track" : L"tracks");
}

}
