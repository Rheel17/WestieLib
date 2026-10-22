#include "ui/framework/Animation.h"

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

Animation::Animation(UINT_PTR timer, UINT interval_ms) noexcept : timer_(timer), interval_ms_(interval_ms) {}

void Animation::run(HWND window, bool running) {
    if (running == running_) {
        return;
    }
    running_ = running;
    if (!running) {
        moving_ = false;
        KillTimer(window, timer_);
        return;
    }

    // With Windows' animations off, it stands still at its start
    BOOL animate = TRUE;
    platform::win32::check(SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animate, 0), "SystemParametersInfoW");
    moving_ = animate != FALSE;
    started_ = std::chrono::steady_clock::now();
    if (moving_) {
        platform::win32::check(SetTimer(window, timer_, interval_ms_, nullptr), "SetTimer");
    }
}

bool Animation::is_tick(WPARAM timer) const noexcept {
    // A tick can still be queued when the animation stops
    return timer == timer_ && running_;
}

double Animation::progress(std::chrono::milliseconds period) const noexcept {
    if (!moving_) {
        return 0;
    }
    // From the clock rather than counted ticks, which come late when the thread is busy
    std::chrono::steady_clock::duration into_period = (std::chrono::steady_clock::now() - started_) % period;
    return std::chrono::duration<double>(into_period) / period;
}

}
