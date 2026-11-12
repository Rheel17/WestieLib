#pragma once

#include <stop_token>
#include <string>
#include <vector>

#include "core/log/Log.h"
#include "core/tools/ToolFolder.h"
#include "platform/FileOps.h"
#include "platform/ProcessRunner.h"

namespace westie::core {

class YtDlp {
public:
    YtDlp(ToolFolder& folder, const platform::FileOps& files, const platform::ProcessRunner& runner, Log& log);

    platform::ProcessResult run(std::vector<std::string> arguments, const std::stop_token& stop) const;

private:
    ToolFolder& folder_;
    const platform::FileOps& files_;
    const platform::ProcessRunner& runner_;
    Log& log_;
};

}
