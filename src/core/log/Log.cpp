#include "core/log/Log.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <exception>
#include <format>
#include <span>
#include <utility>
#include <vector>

#include "core/events/CoreEvent.h"
#include "core/log/LogLine.h"
#include "core/log/LogText.h"

namespace westie::core {

namespace {

constexpr std::size_t KeptLogFiles = 20;

}

Log::Log(platform::FileOps& files, std::filesystem::path folder, const platform::TimeZone& time_zone, EventSink& events) :
        folder_(std::move(folder)), time_zone_(time_zone), events_(events) {
    // A log that can't be written mustn't keep the app from starting; its lines still reach the pane
    try {
        open(files);
    } catch (const std::exception& error) {
        write(LogLevel::Error, std::format("Couldn't open the log file: {}", error.what()));
    }
}

const std::filesystem::path& Log::folder() const noexcept {
    return folder_;
}

void Log::write(LogLevel level, std::string text, LogSubject subject) {
    std::scoped_lock lock(mutex_);
    write_locked(level, std::move(text), std::move(subject));
}

// One lock for the output's lines, so another worker's lines don't come between them
void Log::write_tool_output(std::string_view tool, const platform::ProcessResult& result, const LogSubject& subject) {
    std::scoped_lock lock(mutex_);
    for (std::string& text : tool_output_lines(tool, result)) {
        write_locked(LogLevel::Trace, std::move(text), subject);
    }
}

void Log::write_locked(LogLevel level, std::string text, LogSubject subject) {
    LogLine line{.time = now(), .level = level, .text = std::move(text), .subject = std::move(subject)};
    std::string file_line = log_file_line(line);
    events_.emit(std::move(line));
    append(file_line);
}

void Log::open(platform::FileOps& files) {
    files.create_folders(folder_);
    std::vector<std::filesystem::path> logs;
    for (std::filesystem::path& file : files.list_files(folder_)) {
        if (file.extension() == L".log") {
            logs.push_back(std::move(file));
        }
    }
    // The names start with the time, so the oldest sort first; this run's file makes the last of the ones kept
    std::ranges::sort(logs);
    std::size_t removed = logs.size() < KeptLogFiles ? 0 : logs.size() - (KeptLogFiles - 1);
    for (const std::filesystem::path& old : std::span(logs).first(removed)) {
        // One that is open elsewhere goes at a later start
        try {
            files.remove_file(old);
        } catch (const std::exception&) {
        }
    }
    file_ = files.open_for_append(folder_ / log_file_name(now()));
}

std::chrono::local_time<std::chrono::milliseconds> Log::now() const {
    return time_zone_.to_local(std::chrono::system_clock::now());
}

void Log::append(std::string_view text) {
    if (!file_) {
        return;
    }
    try {
        file_->append(text);
    } catch (const std::exception& error) {
        // Writing stops at the first failure, so a full disk shows as one line in the pane rather than one per line
        file_.reset();
        events_.emit(LogLine{.time = now(), .level = LogLevel::Error, .text = std::format("Couldn't write the log file, so it ends here: {}", error.what())});
    }
}

}
