#include "ui/main/StateRowsWorker.h"

#include <utility>

namespace westie::ui {

StateRowsWorker::StateRowsWorker(const platform::TextNormalizer& normalizer, std::function<void()> on_built) :
        normalizer_(normalizer), on_built_(std::move(on_built)), worker_([this](std::stop_token stop) { work(stop); }) {}

void StateRowsWorker::request(std::shared_ptr<const core::State> state, std::optional<std::string> playlist, std::uint64_t generation) {
    {
        std::scoped_lock lock(mutex_);
        request_ = Request{.state = std::move(state), .playlist = std::move(playlist), .generation = generation};
    }
    wake_.notify_one();
}

std::optional<BuiltStateRows> StateRowsWorker::take() {
    std::scoped_lock lock(mutex_);
    if (failure_) {
        std::rethrow_exception(std::exchange(failure_, nullptr));
    }
    return std::exchange(built_, std::nullopt);
}

void StateRowsWorker::work(const std::stop_token& stop) {
    while (true) {
        Request taken;
        {
            std::unique_lock lock(mutex_);
            if (!wake_.wait(lock, stop, [this] { return request_.has_value(); })) {
                return;
            }
            taken = std::move(*std::exchange(request_, std::nullopt));
        }
        // An exception that leaves a thread ends the process, so the window rethrows it instead
        try {
            StateRows rows = state_rows(std::move(taken.state), std::move(taken.playlist), normalizer_);
            std::scoped_lock lock(mutex_);
            built_ = BuiltStateRows{.generation = taken.generation, .rows = std::move(rows)};
        } catch (...) {
            std::scoped_lock lock(mutex_);
            failure_ = std::current_exception();
        }
        on_built_();
    }
}

}
