#pragma once

#include <optional>
#include <set>
#include <string>

#include "core/state/State.h"

namespace westie::ui {

class RemovalsNotice {
public:
    void update(const core::State& state);
    void dismiss();
    const std::optional<std::wstring>& message() const noexcept;

private:
    std::set<std::string> removed_;
    // The Removed tracks when it was dismissed; another one brings it back
    std::set<std::string> dismissed_;
    std::optional<std::wstring> message_;
};

}
