#pragma once

#include <optional>
#include <vector>

#include <windows.h>

#include "ui/framework/Rect.h"

namespace westie::ui::framework {

class DeferredPositions {
public:
    // Shown at the rect, or hidden without one
    void place(HWND hwnd, const std::optional<Rect>& rect);
    // Shown or hidden as it is
    void move(HWND hwnd, const Rect& rect);
    void apply();

private:
    struct Placement {
        HWND hwnd = nullptr;
        std::optional<Rect> rect{};
        bool keeps_visibility = false;
    };

    static UINT flags_for(const Placement& placement) noexcept;
    bool apply_deferred() const;

    std::vector<Placement> placements_;
};

}
