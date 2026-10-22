#pragma once

#include <concepts>
#include <filesystem>
#include <utility>

#include "core/state/State.h"
#include "platform/FileOps.h"

namespace westie::core {

class StateStore {
public:
    StateStore(platform::FileOps& files, std::filesystem::path file);

    const State& state() const noexcept;

    void update(std::invocable<State&> auto change) {
        State changed = state_;
        change(changed);
        save(changed);
        state_ = std::move(changed);
    }

private:
    State load() const;
    void save(const State& state);

    platform::FileOps& files_;
    std::filesystem::path file_;
    std::filesystem::path backup_;
    State state_;
};

}
