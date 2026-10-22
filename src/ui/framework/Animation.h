#pragma once

#include <chrono>

#include <windows.h>

namespace westie::ui::framework {

class Animation {
public:
    Animation(UINT_PTR timer, UINT interval_ms) noexcept;

    void run(HWND window, bool running);
    bool is_tick(WPARAM timer) const noexcept;
    double progress(std::chrono::milliseconds period) const noexcept;

private:
    UINT_PTR timer_ = 0;
    UINT interval_ms_ = 0;
    bool running_ = false;
    bool moving_ = false;
    std::chrono::steady_clock::time_point started_{};
};

}
