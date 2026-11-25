#pragma once

#include <filesystem>
#include <functional>
#include <stop_token>
#include <string>

#include "core/download/CoverArt.h"
#include "core/download/DownloadedAudio.h"
#include "core/download/Downloader.h"
#include "core/log/Log.h"
#include "core/settings/Settings.h"
#include "core/state/EffectiveMetadata.h"
#include "core/state/State.h"
#include "platform/FileOps.h"

namespace westie::core {


class TrackDownload {
public:
    TrackDownload(const Downloader& downloader, const CoverArt& cover_art, platform::FileOps& files, Log& log);

    TaggedAudio run(
            const State& state, const std::string& track_key, const std::string& ytm_id, const Settings& settings, const std::filesystem::path& work,
            const std::function<void(int percent)>& on_percent, const std::stop_token& stop
    ) const;

private:
    std::string cover(
            const State& state, const std::string& track_key, const TrackMetadata& metadata, const DownloadedAudio& audio, const std::filesystem::path& work,
            const std::stop_token& stop
    ) const;

    const Downloader& downloader_;
    const CoverArt& cover_art_;
    platform::FileOps& files_;
    Log& log_;
};

}
