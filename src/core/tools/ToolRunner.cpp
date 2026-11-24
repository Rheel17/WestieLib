#include "core/tools/ToolRunner.h"

#include <filesystem>
#include <format>
#include <shared_mutex>

#include "core/log/LogLevel.h"
#include "core/log/LogText.h"
#include "core/tools/ToolMissing.h"

namespace westie::core {

ToolRunner::ToolRunner(ToolFolder& folder, const platform::FileOps& files, const platform::ProcessRunner& runner, Log& log) :
        folder_(folder), files_(files), runner_(runner), log_(log) {}

platform::ProcessResult ToolRunner::run(
        ToolPackage package, const std::vector<std::string>& arguments, const platform::OutputLineHandler& on_output_line, const std::stop_token& stop
) const {
    // Held while it runs, so an update waits to replace it
    std::shared_lock in_use = folder_.lock_for_use(package);
    std::filesystem::path program = folder_.program(package);
    if (!files_.is_file(program)) {
        throw ToolMissing(package);
    }
    platform::ProcessResult result = runner_.run(program, arguments, on_output_line, stop);
    std::string_view name = tool_package_name(package);
    log_.write_tool_run(name, arguments, result);
    log_.write(LogLevel::Debug, std::format("{}: exit code {}", command_line(name, arguments), result.exit_code));
    return result;
}

}
