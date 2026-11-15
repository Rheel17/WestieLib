#pragma once

#include <chrono>
#include <deque>
#include <mutex>

namespace westie::core {

class SearchTimeout {
public:
    std::chrono::milliseconds current() const;
    void answered(std::chrono::milliseconds took);
    void timed_out();

private:
    mutable std::mutex mutex_;
    std::deque<std::chrono::milliseconds> answers_;
    int timeouts_in_a_row_ = 0;
};

}
