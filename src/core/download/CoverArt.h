#pragma once

#include <filesystem>
#include <optional>
#include <stop_token>
#include <string>

#include "core/log/LogSubject.h"
#include "core/tools/Ffmpeg.h"
#include "platform/FileOps.h"
#include "platform/HttpClient.h"

namespace westie::core {

class CoverArt {
public:
    CoverArt(platform::HttpClient& http, const Ffmpeg& ffmpeg, const platform::FileOps& files);

    std::string fetch(
            const std::string& cover_url, const std::optional<std::filesystem::path>& thumbnail, const std::filesystem::path& work, const LogSubject& subject,
            const std::stop_token& stop
    ) const;

private:
    platform::HttpClient& http_;
    const Ffmpeg& ffmpeg_;
    const platform::FileOps& files_;
};

}
