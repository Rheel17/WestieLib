#include "core/tools/ToolRunner.h"

#include <filesystem>
#include <shared_mutex>
#include <string_view>

#include "core/log/LogLevel.h"
#include "core/log/LogText.h"
#include "core/tools/ToolMissing.h"

namespace westie::core {

ToolRunner::ToolRunner(ToolFolder& folder, const platform::FileOps& files, const platform::ProcessRunner& runner, Log& log) :
        folder_(folder), files_(files), runner_(runner), log_(log) {}

platform::ProcessResult ToolRunner::run(
        ToolPackage package, const std::vector<std::string>& arguments, const LogSubject& subject, const platform::OutputLineHandler& on_output_line,
        const std::stop_token& stop
) const {
    return run(package, {}, arguments, subject, on_output_line, stop);
}

platform::ProcessResult ToolRunner::run(
        ToolPackage package, std::span<const ToolPackage> also_uses, const std::vector<std::string>& arguments, const LogSubject& subject,
        const platform::OutputLineHandler& on_output_line, const std::stop_token& stop
) const {
    // Held while it runs, so an update waits to replace them
    std::vector<std::shared_lock<std::shared_mutex>> in_use;
    in_use.push_back(folder_.lock_for_use(package));
    for (ToolPackage used : also_uses) {
        in_use.push_back(folder_.lock_for_use(used));
    }
    if (!files_.is_file(folder_.program(package))) {
        throw ToolMissing(package);
    }
    for (ToolPackage used : also_uses) {
        if (!files_.is_file(folder_.program(used))) {
            throw ToolMissing(used);
        }
    }

    std::string_view name = tool_package_name(package);
    // Before it runs, so the lines that come meanwhile show how long it takes
    log_.write(LogLevel::Trace, command_line(name, arguments), subject);
    platform::ProcessResult result = runner_.run(folder_.program(package), arguments, on_output_line, stop);
    log_.write_tool_output(name, result, subject);
    return result;
}

}
