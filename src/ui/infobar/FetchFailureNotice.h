#pragma once

#include <chrono>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ui/playlists/PlaylistRow.h"

namespace westie::ui {

class FetchFailureNotice {
public:
    void update(const std::vector<PlaylistRow>& rows);
    void dismiss();
    const std::optional<std::wstring>& message() const noexcept;

private:
    using Failure = std::pair<std::string, std::chrono::system_clock::time_point>;

    // A failure stays dismissed only until the playlist fails again
    std::set<Failure> dismissed_;
    std::optional<Failure> shown_;
    std::optional<std::wstring> message_;
};

}
