#include <chrono>
#include <format>
#include <source_location>

#include "Checks.h"
#include "UnitChecks.h"
#include "core/matching/SearchTimeout.h"

namespace westie::tests {

namespace {

void check_timeout(const core::SearchTimeout& timeout, int expected_ms, std::source_location where = std::source_location::current()) {
    std::chrono::milliseconds actual = timeout.current();
    check(actual == std::chrono::milliseconds(expected_ms), std::format("{} ms, expected {} ms", actual.count(), expected_ms), where);
}

void answer(core::SearchTimeout& timeout, int count, int took_ms) {
    for (int i = 0; i < count; ++i) {
        timeout.answered(std::chrono::milliseconds(took_ms));
    }
}

void check_calibration() {
    core::SearchTimeout timeout;
    check_timeout(timeout, 4000);
    answer(timeout, 9, 600);
    check_timeout(timeout, 4000);
    answer(timeout, 1, 600);
    check_timeout(timeout, 1200);
    answer(timeout, 1, 1100);
    check_timeout(timeout, 2200);
    // The slow answer leaves the recent ones after 50 more
    answer(timeout, 49, 600);
    check_timeout(timeout, 2200);
    answer(timeout, 1, 600);
    check_timeout(timeout, 1200);
}

void check_bounds() {
    core::SearchTimeout fast;
    answer(fast, 10, 100);
    check_timeout(fast, 1000);
    core::SearchTimeout slow;
    answer(slow, 10, 3000);
    check_timeout(slow, 4000);
}

void check_timeouts() {
    core::SearchTimeout timeout;
    answer(timeout, 10, 600);
    timeout.timed_out();
    timeout.timed_out();
    check_timeout(timeout, 1200);
    // An answer between them starts the count again
    answer(timeout, 1, 600);
    timeout.timed_out();
    timeout.timed_out();
    check_timeout(timeout, 1200);
    timeout.timed_out();
    check_timeout(timeout, 4000);
    answer(timeout, 10, 700);
    check_timeout(timeout, 1400);
}

}

void check_search_timeout() {
    check_calibration();
    check_bounds();
    check_timeouts();
}

}
