#include "core/tools/Ffmpeg.h"

#include <array>
#include <string_view>

#include "core/tools/ToolPackage.h"

namespace westie::core {

namespace {

// No banner, only errors, and never a question on stdin, which is NUL anyway
constexpr std::array<std::string_view, 4> QuietArguments = {"-hide_banner", "-loglevel", "error", "-nostdin"};

}

Ffmpeg::Ffmpeg(const ToolRunner& runner) : runner_(runner) {}

platform::ProcessResult Ffmpeg::run(std::vector<std::string> arguments, const LogSubject& subject, const std::stop_token& stop) const {
    arguments.insert(arguments.begin(), QuietArguments.begin(), QuietArguments.end());
    return runner_.run(ToolPackage::Ffmpeg, arguments, subject, {}, stop);
}

}
