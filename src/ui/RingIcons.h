#pragma once

#include <vector>

#include "ui/framework/IconResource.h"

namespace westie::ui {

// The progress ring, a blue arc that fills clockwise from 12 o'clock, in its 25 steps at one size
class RingIcons {
public:
    explicit RingIcons(int size);

    const framework::IconResource& for_percent(int percent) const noexcept;
    const framework::IconResource& full() const noexcept;

private:
    std::vector<framework::IconResource> rings_;
};

}
