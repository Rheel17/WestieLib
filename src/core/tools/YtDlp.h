#pragma once

#include <span>
#include <stop_token>
#include <string>
#include <vector>

#include "core/tools/ToolPackage.h"
#include "core/tools/ToolRunner.h"
#include "platform/ProcessRunner.h"

namespace westie::core {

class YtDlp {
public:
    explicit YtDlp(const ToolRunner& runner);

    platform::ProcessResult run(std::vector<std::string> arguments, const std::stop_token& stop) const;
    platform::ProcessResult run(
            std::vector<std::string> arguments, std::span<const ToolPackage> also_uses, const platform::OutputLineHandler& on_output_line,
            const std::stop_token& stop
    ) const;

private:
    const ToolRunner& runner_;
};

}
