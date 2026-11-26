#pragma once

#include <deque>
#include <optional>
#include <set>
#include <string>

#include "core/state/State.h"

namespace westie::ui {

// It needs review, or the Matcher found nothing for it, and it has an entry to choose a match for
bool needs_choice(const core::State& state, const std::string& track);

class MatchChoices {
public:
    // A new update offers every track again
    void start_update();
    // A track the update matched or failed to; it waits when it needs a choice and wasn't offered in this update
    void add(const core::State& state, const std::string& track);
    // The next one that still needs a choice; the ones before it that no longer do are dropped
    std::optional<std::string> take(const core::State& state);
    bool any_waiting(const core::State& state) const;
    void clear();

private:
    std::deque<std::string> waiting_;
    std::set<std::string> offered_;
};

}
