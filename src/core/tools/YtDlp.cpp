#include "core/tools/YtDlp.h"

#include <string_view>
#include <utility>

namespace westie::core {

namespace {

// A user's own yt-dlp configuration could change what it prints
constexpr std::string_view IgnoreConfig = "--ignore-config";

}

YtDlp::YtDlp(const ToolRunner& runner) : runner_(runner) {}

platform::ProcessResult YtDlp::run(std::vector<std::string> arguments, const std::stop_token& stop) const {
    return run(std::move(arguments), {}, {}, stop);
}

platform::ProcessResult YtDlp::run(
        std::vector<std::string> arguments, std::span<const ToolPackage> also_uses, const platform::OutputLineHandler& on_output_line,
        const std::stop_token& stop
) const {
    arguments.emplace(arguments.begin(), IgnoreConfig);
    return runner_.run(ToolPackage::YtDlp, also_uses, arguments, on_output_line, stop);
}

}
