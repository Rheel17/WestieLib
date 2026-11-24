#pragma once

#include <stop_token>
#include <string>
#include <vector>

#include "core/tools/ToolRunner.h"
#include "platform/ProcessRunner.h"

namespace westie::core {

class Ffmpeg {
public:
    explicit Ffmpeg(const ToolRunner& runner);

    platform::ProcessResult run(std::vector<std::string> arguments, const std::stop_token& stop) const;

private:
    const ToolRunner& runner_;
};

}
