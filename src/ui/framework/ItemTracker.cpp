#include "ui/framework/ItemTracker.h"

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

std::optional<std::size_t> ItemTracker::hot() const noexcept {
    return hot_;
}

std::optional<std::size_t> ItemTracker::pressed() const noexcept {
    return hot_ == pressed_ ? pressed_ : std::nullopt;
}

bool ItemTracker::move(HWND window, std::optional<std::size_t> item) {
    track_leave(window);
    // While the button is held, only the pressed item reacts
    std::optional<std::size_t> hot = pressed_ && item != pressed_ ? std::nullopt : item;
    if (hot == hot_) {
        return false;
    }
    hot_ = hot;
    return true;
}

bool ItemTracker::leave() noexcept {
    // The capture keeps the moves coming, so they decide
    if (pressed_ || !hot_) {
        return false;
    }
    hot_.reset();
    return true;
}

bool ItemTracker::press(HWND window, std::optional<std::size_t> item) {
    if (!item) {
        return false;
    }
    pressed_ = item;
    hot_ = item;
    SetCapture(window);
    return true;
}

std::optional<std::size_t> ItemTracker::release(HWND window, std::optional<std::size_t> item) {
    if (!pressed_) {
        return std::nullopt;
    }
    std::optional<std::size_t> clicked = item == pressed_ ? pressed_ : std::nullopt;
    // Cleared first, so the WM_CAPTURECHANGED that ReleaseCapture sends finds nothing to cancel
    pressed_.reset();
    hot_ = item;
    platform::win32::check(ReleaseCapture(), "ReleaseCapture");
    // A mouse that leaves without moving first must still take the hover along
    track_leave(window);
    return clicked;
}

bool ItemTracker::cancel() noexcept {
    if (!pressed_) {
        return false;
    }
    pressed_.reset();
    hot_.reset();
    return true;
}

void ItemTracker::track_leave(HWND window) {
    TRACKMOUSEEVENT track{.cbSize = sizeof(TRACKMOUSEEVENT), .dwFlags = TME_LEAVE, .hwndTrack = window, .dwHoverTime = 0};
    platform::win32::check(TrackMouseEvent(&track), "TrackMouseEvent");
}

}
