#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "core/download/DownloadedAudio.h"
#include "core/state/State.h"

namespace westie::core {

bool apply_download(State& state, const std::string& track_key, const std::string& ytm_id, const std::filesystem::path& file, const VideoInfo& info);
void apply_download_failure(State& state, const std::string& track_key, std::string error);
std::vector<std::string> apply_retry(State& state, const std::vector<std::string>& track_keys);

}
