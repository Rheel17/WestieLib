#include "ui/framework/DeferredPositions.h"

namespace westie::ui::framework {

namespace {

// Copied bits can come from a spot another window in the same batch has just moved into
constexpr UINT PlacementFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS;

}

void DeferredPositions::move(HWND hwnd, const Rect& rect) {
    placements_.push_back({hwnd, rect});
}

void DeferredPositions::apply() {
    if (placements_.empty()) {
        return;
    }

    // A failed DeferWindowPos drops the whole batch
    if (!apply_deferred()) {
        for (const Placement& placement : placements_) {
            const Rect& rect = placement.rect;
            SetWindowPos(placement.hwnd, nullptr, rect.left, rect.top, rect.width(), rect.height(), PlacementFlags);
        }
    }

    // Mouse input outranks WM_PAINT, so during a drag the windows would wait to be painted
    RedrawWindow(GetParent(placements_.front().hwnd), nullptr, nullptr, RDW_UPDATENOW | RDW_ALLCHILDREN);
    placements_.clear();
}

bool DeferredPositions::apply_deferred() const {
    HDWP positions = BeginDeferWindowPos(static_cast<int>(placements_.size()));
    for (const Placement& placement : placements_) {
        if (!positions) {
            return false;
        }
        const Rect& rect = placement.rect;
        positions = DeferWindowPos(positions, placement.hwnd, nullptr, rect.left, rect.top, rect.width(), rect.height(), PlacementFlags);
    }
    return positions && EndDeferWindowPos(positions);
}

}
