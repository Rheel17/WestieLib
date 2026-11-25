#include "ui/RingIcons.h"

#include <algorithm>

#include "ResourceIds.h"

namespace westie::ui {

namespace {

constexpr int RingSteps = 24;
constexpr int FullPercent = 100;

}

RingIcons::RingIcons(int size) {
    for (int step = 0; step <= RingSteps; ++step) {
        rings_.emplace_back(IDI_RING_0 + step, size);
    }
}

const framework::IconResource& RingIcons::for_percent(int percent) const noexcept {
    int step = (std::clamp(percent, 0, FullPercent) * RingSteps + FullPercent / 2) / FullPercent;
    return rings_[step];
}

const framework::IconResource& RingIcons::full() const noexcept {
    return rings_.back();
}

}
