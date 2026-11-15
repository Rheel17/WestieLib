#include "core/matching/SearchTimeout.h"

#include <algorithm>
#include <cstddef>

namespace westie::core {

namespace {

// YouTube Music takes 5 s or more to refuse a search, so even before it knows the connection, a search gives up before
// a refusal arrives
constexpr std::chrono::milliseconds StartTimeout{4000};
constexpr std::chrono::milliseconds MinTimeout{1000};
// The slowest of the recent answers, twice over
constexpr int Margin = 2;
constexpr std::size_t RecentAnswers = 50;
constexpr std::size_t AnswersToCalibrate = 10;
// Refusals mostly come one or two at a time, so three timeouts in a row more likely mean the connection got slower
constexpr int TimeoutsToRecalibrate = 3;

}

std::chrono::milliseconds SearchTimeout::current() const {
    std::scoped_lock lock(mutex_);
    if (answers_.size() < AnswersToCalibrate) {
        return StartTimeout;
    }
    return std::clamp(std::ranges::max(answers_) * Margin, MinTimeout, StartTimeout);
}

void SearchTimeout::answered(std::chrono::milliseconds took) {
    std::scoped_lock lock(mutex_);
    answers_.push_back(took);
    if (answers_.size() > RecentAnswers) {
        answers_.pop_front();
    }
    timeouts_in_a_row_ = 0;
}

void SearchTimeout::timed_out() {
    std::scoped_lock lock(mutex_);
    if (++timeouts_in_a_row_ >= TimeoutsToRecalibrate) {
        answers_.clear();
        timeouts_in_a_row_ = 0;
    }
}

}
