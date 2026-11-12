#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>

#include "core/events/EventSink.h"
#include "core/log/LogLevel.h"
#include "core/log/LogSubject.h"
#include "platform/AppendFile.h"
#include "platform/FileOps.h"
#include "platform/ProcessRunner.h"
#include "platform/TimeZone.h"

namespace westie::core {

class Log {
public:
    Log(platform::FileOps& files, std::filesystem::path folder, const platform::TimeZone& time_zone, EventSink& events);

    const std::filesystem::path& folder() const noexcept;
    void write(LogLevel level, std::string text, LogSubject subject = {});
    void write_tool_run(std::string_view tool, std::span<const std::string> arguments, const platform::ProcessResult& result);

private:
    void open(platform::FileOps& files);
    std::chrono::local_time<std::chrono::milliseconds> now() const;
    void append(std::string_view text);

    std::filesystem::path folder_;
    const platform::TimeZone& time_zone_;
    EventSink& events_;
    // Keeps lines whole and in time order, in the file and in the events alike
    std::mutex mutex_;
    std::unique_ptr<platform::AppendFile> file_;
};

}
