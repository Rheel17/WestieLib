#include "ui/framework/PendingException.h"

#include <utility>

namespace westie::ui::framework {

thread_local std::exception_ptr PendingException::pending_;

void PendingException::capture() noexcept {
    if (!pending_) {
        pending_ = std::current_exception();
    }
}

void PendingException::rethrow_if_any() {
    if (pending_) {
        std::rethrow_exception(std::exchange(pending_, nullptr));
    }
}

}
