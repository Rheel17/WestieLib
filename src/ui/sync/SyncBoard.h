#pragma once

#include <optional>
#include <span>
#include <string>
#include <vector>

#include "core/sync/PlaylistFetching.h"
#include "core/sync/SyncQueued.h"
#include "core/tools/ToolPackage.h"

namespace westie::ui {

class SyncBoard {
public:
    void apply(const core::SyncQueued& queued);
    void apply(const core::PlaylistFetching& fetching);
    void finish_playlist(const std::string& playlist);
    void finish();

    bool running() const noexcept;
    bool queued(const std::string& playlist) const;
    const std::optional<std::string>& current() const noexcept;
    std::span<const core::ToolPackage> waiting_for() const noexcept;

private:
    bool running_ = false;
    std::vector<std::string> queue_;
    std::optional<std::string> current_;
    std::vector<core::ToolPackage> waiting_for_;
};

}
