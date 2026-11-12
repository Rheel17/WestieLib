#include "core/tools/YtDlp.h"

#include <filesystem>
#include <format>
#include <shared_mutex>
#include <string_view>

#include "core/log/LogLevel.h"
#include "core/log/LogText.h"
#include "core/tools/ToolMissing.h"

namespace westie::core {

namespace {

constexpr std::string_view Name = "yt-dlp";
// A user's own yt-dlp configuration could change what it prints
constexpr std::string_view IgnoreConfig = "--ignore-config";

}

YtDlp::YtDlp(ToolFolder& folder, const platform::FileOps& files, const platform::ProcessRunner& runner, Log& log) :
        folder_(folder), files_(files), runner_(runner), log_(log) {}

platform::ProcessResult YtDlp::run(std::vector<std::string> arguments, const std::stop_token& stop) const {
    // Held while it runs, so an update waits to replace it
    std::shared_lock in_use = folder_.lock_for_use(ToolPackage::YtDlp);
    std::filesystem::path program = folder_.program(ToolPackage::YtDlp);
    if (!files_.is_file(program)) {
        throw ToolMissing(ToolPackage::YtDlp);
    }
    arguments.emplace(arguments.begin(), IgnoreConfig);
    platform::ProcessResult result = runner_.run(program, arguments, stop);
    log_.write_tool_run(Name, arguments, result);
    log_.write(LogLevel::Debug, std::format("{}: exit code {}", command_line(Name, arguments), result.exit_code));
    return result;
}

}
