#include "core/tools/YtDlp.h"

#include <filesystem>
#include <shared_mutex>
#include <string_view>

#include "core/tools/ToolMissing.h"

namespace westie::core {

namespace {

// A user's own yt-dlp configuration could change what it prints
constexpr std::string_view IgnoreConfig = "--ignore-config";

}

YtDlp::YtDlp(ToolFolder& folder, const platform::FileOps& files, const platform::ProcessRunner& runner) : folder_(folder), files_(files), runner_(runner) {}

platform::ProcessResult YtDlp::run(std::vector<std::string> arguments, const std::stop_token& stop) const {
    // Held while it runs, so an update waits to replace it
    std::shared_lock in_use = folder_.lock_for_use(ToolPackage::YtDlp);
    std::filesystem::path program = folder_.program(ToolPackage::YtDlp);
    if (!files_.is_file(program)) {
        throw ToolMissing(ToolPackage::YtDlp);
    }
    arguments.emplace(arguments.begin(), IgnoreConfig);
    return runner_.run(program, arguments, stop);
}

}
