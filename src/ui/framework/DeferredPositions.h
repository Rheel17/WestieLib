#pragma once

#include <optional>
#include <vector>

#include <windows.h>

#include "ui/framework/Rect.h"

namespace westie::ui::framework {

class DeferredPositions {
public:
    void place(HWND hwnd, const std::optional<Rect>& rect);
    void apply();

private:
    struct Placement {
        HWND hwnd;
        std::optional<Rect> rect;
    };

    static UINT flags_for(const Placement& placement) noexcept;
    bool apply_deferred() const;

    std::vector<Placement> placements_;
};

}
