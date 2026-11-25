#include "core/library/ResolveRemovals.h"

#include <memory>

#include "core/log/LogLevel.h"
#include "core/log/LogSubject.h"

namespace westie::core {

void resolve_removals(Library& library, Log& log, const std::map<std::string, RemovalDecision>& decisions) {
    std::map<std::string, RemovalDecision> applied;
    library.update([&](State& state) { applied = apply_removals(state, decisions); });
    // The Library logs each file it deletes
    std::shared_ptr<const State> state = library.snapshot();
    for (const auto& [key, decision] : applied) {
        if (decision == RemovalDecision::Keep) {
            log.write(LogLevel::Info, "Kept the file", track_subject(*state, key));
        }
    }
}

}