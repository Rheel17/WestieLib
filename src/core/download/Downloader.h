#pragma once

#include <filesystem>
#include <functional>
#include <stop_token>
#include <string>

#include "core/download/DownloadedAudio.h"
#include "core/log/LogSubject.h"
#include "core/settings/Settings.h"
#include "core/tools/ToolFolder.h"
#include "core/tools/YtDlp.h"
#include "platform/FileOps.h"

namespace westie::core {

class Downloader {
public:
    Downloader(const YtDlp& yt_dlp, const ToolFolder& tools, const platform::FileOps& files);

    DownloadedAudio download(
            const std::string& ytm_id, AudioFormat format, const std::filesystem::path& work, const LogSubject& subject,
            const std::function<void(int percent)>& on_percent, const std::stop_token& stop
    ) const;

private:
    VideoInfo read_info(const std::filesystem::path& file) const;

    const YtDlp& yt_dlp_;
    const ToolFolder& tools_;
    const platform::FileOps& files_;
};

}
