#pragma once

#include <concepts>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "core/download/DownloadedAudio.h"
#include "core/library/FileChanges.h"
#include "core/log/Log.h"
#include "core/log/LogSubject.h"
#include "core/settings/SettingsStore.h"
#include "core/state/StateStore.h"
#include "platform/FileOps.h"

namespace westie::core {

class Library {
public:
    Library(StateStore& state, platform::FileOps& files, const SettingsStore& settings, Log& log);

    std::shared_ptr<const State> snapshot() const noexcept;

    void update(std::invocable<State&> auto change) {
        std::scoped_lock lock(mutex_);
        update_locked(change);
    }

    // None when the track left the state, or got another YouTube Music id, while it downloaded
    std::optional<std::filesystem::path> place(const std::string& track_key, const std::string& ytm_id, const TaggedAudio& audio);

private:
    void update_locked(std::invocable<State&> auto& change) {
        std::shared_ptr<const State> before;
        state_.update([&](State& state) {
            before = state_.snapshot();
            change(state);
        });
        settle(*before, *state_.snapshot());
    }

    std::filesystem::path move_in(const State& state, const std::string& track_key, const TaggedAudio& audio, const std::filesystem::path& library);
    void settle(const State& before, const State& after);
    void drop(const DroppedFile& dropped, const State& before, const std::filesystem::path& library);
    void retag(const RetaggedFile& retagged, const State& after, const std::filesystem::path& library);
    void rename(const RetaggedFile& retagged, const LogSubject& subject, const std::filesystem::path& library);

    StateStore& state_;
    platform::FileOps& files_;
    const SettingsStore& settings_;
    Log& log_;
    // A change and the file work it brings never interleave with another's
    std::mutex mutex_;
};

}
