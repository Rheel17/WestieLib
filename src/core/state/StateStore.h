#pragma once

#include <atomic>
#include <concepts>
#include <filesystem>
#include <memory>
#include <mutex>
#include <utility>

#include "core/state/State.h"
#include "platform/FileOps.h"

namespace westie::core {

class StateStore {
public:
    StateStore(platform::FileOps& files, std::filesystem::path file);

    std::shared_ptr<const State> snapshot() const noexcept;

    void update(std::invocable<State&> auto change) {
        // Changes from the UI and the sync never interleave, and a reader never sees one halfway
        std::scoped_lock updating(update_mutex_);
        State changed = *snapshot();
        change(changed);
        save(changed);
        state_.store(std::make_shared<const State>(std::move(changed)));
    }

private:
    State load() const;
    void save(const State& state);

    platform::FileOps& files_;
    std::filesystem::path file_;
    std::filesystem::path backup_;
    std::mutex update_mutex_;
    std::atomic<std::shared_ptr<const State>> state_;
};

}
