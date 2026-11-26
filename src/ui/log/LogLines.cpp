#include "ui/log/LogLines.h"

#include <algorithm>
#include <string_view>
#include <utility>

#include "core/log/LogText.h"
#include "platform/win32/Utf16.h"

namespace westie::ui {

namespace {

constexpr std::size_t KeptLines = 20'000;
constexpr wchar_t NoSubject[] = L"—";

std::wstring wide(std::string_view text) {
    return platform::win32::to_utf16(text);
}

// A row is one line in the list
std::wstring one_line(std::wstring text) {
    std::ranges::replace(text, L'\r', L' ');
    std::ranges::replace(text, L'\n', L' ');
    return text;
}

}

void LogLines::append(const core::LogLine& line) {
    rows_.push_back({
        .level = line.level,
        .subject = line.subject,
        .time = wide(core::format_log_time(line.time)),
        .level_text = wide(core::log_level_name(line.level)),
        .subject_text = line.subject.name.empty() ? NoSubject : wide(line.subject.name),
        .text = one_line(wide(line.text)),
    });
    if (log_filter_shows(filter_, line.level, line.subject)) {
        shown_.push_back(dropped_ + rows_.size() - 1);
    }
    if (rows_.size() > KeptLines) {
        rows_.pop_front();
        if (!shown_.empty() && shown_.front() == dropped_) {
            shown_.pop_front();
        }
        ++dropped_;
    }
}

const LogFilter& LogLines::filter() const noexcept {
    return filter_;
}

bool LogLines::set_filter(LogFilter filter) {
    bool differ = log_filters_differ(filter_, filter);
    filter_ = std::move(filter);
    if (differ) {
        show_all();
    }
    return differ;
}

void LogLines::rename_track(const std::string& from, const std::string& to) {
    for (LogRow& row : rows_) {
        if (row.subject.track == from) {
            row.subject.track = to;
        }
    }
    if (filter_.view.scope != LogScope::All) {
        show_all();
    }
}

std::size_t LogLines::shown_count() const noexcept {
    return shown_.size();
}

const LogRow& LogLines::shown(std::size_t index) const {
    return rows_.at(shown_.at(index) - dropped_);
}

std::size_t LogLines::line_number(std::size_t index) const {
    return shown_.at(index);
}

std::optional<std::size_t> LogLines::shown_index(std::size_t line_number) const {
    auto found = std::ranges::lower_bound(shown_, line_number);
    if (found == shown_.end() || *found != line_number) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(found - shown_.begin());
}

void LogLines::show_all() {
    shown_.clear();
    for (std::size_t index = 0; index < rows_.size(); ++index) {
        if (log_filter_shows(filter_, rows_[index].level, rows_[index].subject)) {
            shown_.push_back(dropped_ + index);
        }
    }
}

}
