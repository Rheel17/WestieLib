#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <string>

#include "core/log/LogLevel.h"
#include "core/log/LogLine.h"
#include "core/log/LogSubject.h"
#include "ui/log/LogFilter.h"

namespace westie::ui {

struct LogRow {
    core::LogLevel level = core::LogLevel::Info;
    core::LogSubject subject{};
    std::wstring time{};
    std::wstring level_text{};
    std::wstring subject_text{};
    std::wstring text{};
};

class LogLines {
public:
    void append(const core::LogLine& line);
    const LogFilter& filter() const noexcept;
    // Whether the lines shown changed
    bool set_filter(LogFilter filter);
    void rename_track(const std::string& from, const std::string& to);
    std::size_t shown_count() const noexcept;
    const LogRow& shown(std::size_t index) const;
    // A line's number stays with it when the lines shown change
    std::size_t line_number(std::size_t index) const;
    std::optional<std::size_t> shown_index(std::size_t line_number) const;

private:
    void show_all();

    std::deque<LogRow> rows_;
    // Numbered from the first line ever, so the numbers stay put when the oldest lines go
    std::deque<std::size_t> shown_;
    std::size_t dropped_ = 0;
    LogFilter filter_;
};

}
