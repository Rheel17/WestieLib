#pragma once

#include <optional>
#include <set>
#include <string>

#include "core/log/LogLevel.h"
#include "core/log/LogSubject.h"

namespace westie::ui {

enum class LogScope { All, Playlist, Track };

struct LogView {
    // The least severe level shown
    core::LogLevel least = core::LogLevel::Info;
    LogScope scope = LogScope::All;

    bool operator==(const LogView&) const = default;
};

struct LogSelection {
    // None while All tracks is selected
    std::optional<std::string> playlist{};
    // The tracks the selected playlist lists now
    std::set<std::string> playlist_tracks{};
    std::set<std::string> tracks{};

    bool operator==(const LogSelection&) const = default;
};

struct LogFilter {
    LogView view{};
    LogSelection selection{};
};

bool log_filter_shows(const LogFilter& filter, core::LogLevel level, const core::LogSubject& subject);
// Whether the filters show different lines, or might
bool log_filters_differ(const LogFilter& a, const LogFilter& b);

}
