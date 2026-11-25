#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "core/state/State.h"
#include "core/tagging/TrackTags.h"

namespace westie::core {

struct DroppedFile {
    std::filesystem::path file{};
    // The track that had it, in the state before
    std::string track{};
};

struct RetaggedFile {
    std::filesystem::path file{};
    // The track that has it, in the state after
    std::string track{};
    TrackTags tags{};
    // Set when the file's name should change
    std::optional<std::string> stem{};
};

struct FileChanges {
    std::vector<DroppedFile> dropped{};
    std::vector<RetaggedFile> retagged{};
};

TrackTags track_tags(const State& state, const Track& track);
FileChanges file_changes(const State& before, const State& after);

}
