#include "ui/events/CoreEventQueue.h"

#include <utility>

namespace westie::ui {

void CoreEventQueue::set_target(HWND target) {
    std::scoped_lock lock(mutex_);
    target_ = target;
    if (target_ && !events_.empty()) {
        PostMessageW(target_, CoreEventsMessage, 0, 0);
    }
}

void CoreEventQueue::emit(core::CoreEvent event) {
    std::scoped_lock lock(mutex_);
    events_.push_back(std::move(event));
    // One message wakes the window for everything queued until it takes the events. A failed post leaves them for
    // the next target.
    if (target_ && events_.size() == 1) {
        PostMessageW(target_, CoreEventsMessage, 0, 0);
    }
}

std::vector<core::CoreEvent> CoreEventQueue::take() {
    std::scoped_lock lock(mutex_);
    return std::exchange(events_, {});
}

}
