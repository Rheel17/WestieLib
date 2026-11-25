#pragma once

#include <span>
#include <stop_token>
#include <string>
#include <vector>

#include "core/log/Log.h"
#include "core/log/LogSubject.h"
#include "core/tools/ToolFolder.h"
#include "core/tools/ToolPackage.h"
#include "platform/FileOps.h"
#include "platform/ProcessRunner.h"

namespace westie::core {

class ToolRunner {
public:
    ToolRunner(ToolFolder& folder, const platform::FileOps& files, const platform::ProcessRunner& runner, Log& log);

    platform::ProcessResult run(
            ToolPackage package, const std::vector<std::string>& arguments, const LogSubject& subject, const platform::OutputLineHandler& on_output_line,
            const std::stop_token& stop
    ) const;
    // For a tool that runs the others in turn, which must stay in place while it runs
    platform::ProcessResult run(
            ToolPackage package, std::span<const ToolPackage> also_uses, const std::vector<std::string>& arguments, const LogSubject& subject,
            const platform::OutputLineHandler& on_output_line, const std::stop_token& stop
    ) const;

private:
    ToolFolder& folder_;
    const platform::FileOps& files_;
    const platform::ProcessRunner& runner_;
    Log& log_;
};

}
