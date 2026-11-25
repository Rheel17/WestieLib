#pragma once

#include <condition_variable>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>

#include "core/state/State.h"
#include "platform/TextNormalizer.h"
#include "ui/main/StateRows.h"

namespace westie::ui {

struct BuiltStateRows {
    // The request's, to tell rows of an older request from the current ones
    std::uint64_t generation = 0;
    StateRows rows{};
};

class StateRowsWorker {
public:
    // on_built runs on the worker's thread
    StateRowsWorker(const platform::TextNormalizer& normalizer, std::function<void()> on_built);

    void request(std::shared_ptr<const core::State> state, std::optional<std::string> playlist, std::uint64_t generation);
    // Rethrows what building the rows threw
    std::optional<BuiltStateRows> take();

private:
    struct Request {
        std::shared_ptr<const core::State> state{};
        std::optional<std::string> playlist{};
        std::uint64_t generation = 0;
    };

    void work(const std::stop_token& stop);

    const platform::TextNormalizer& normalizer_;
    std::function<void()> on_built_;
    std::mutex mutex_;
    std::condition_variable_any wake_;
    // Only the newest request is built
    std::optional<Request> request_;
    std::optional<BuiltStateRows> built_;
    std::exception_ptr failure_;
    // Last, so the thread is joined before the members it uses go
    std::jthread worker_;
};

}
