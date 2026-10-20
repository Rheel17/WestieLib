#pragma once

#include <vector>

#include <windows.h>

#include "ui/framework/Rect.h"

namespace westie::ui::framework {

class DeferredPositions {
public:
    void move(HWND hwnd, const Rect& rect);
    void apply();

private:
    struct Placement {
        HWND hwnd;
        Rect rect;
    };

    bool apply_deferred() const;

    std::vector<Placement> placements_;
};

}
