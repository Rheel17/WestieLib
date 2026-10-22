#include "ui/framework/FocusMemory.h"

namespace westie::ui::framework {

void FocusMemory::save(HWND window) noexcept {
    HWND focus = GetFocus();
    focus_ = IsChild(window, focus) ? focus : nullptr;
}

void FocusMemory::restore(HWND window) const noexcept {
    // The child may have been hidden or destroyed while the window was inactive
    if (focus_ != nullptr && IsChild(window, focus_) && IsWindowVisible(focus_) && IsWindowEnabled(focus_)) {
        SetFocus(focus_);
    }
}

}
