#include "core/library/Library.h"

#include <exception>
#include <format>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "core/json/Utf8Path.h"
#include "core/library/LibraryFileName.h"
#include "core/library/WindowsNames.h"
#include "core/log/LogLevel.h"
#include "core/log/LogText.h"
#include "core/state/TrackYtmIds.h"
#include "core/sync/ApplyDownload.h"
#include "core/tagging/Tagger.h"

namespace westie::core {

namespace {

// Each try finds a name free; only names taken between the look and the move cost another
constexpr int PlacementTries = 100;

// Windows would create another folder than the one the settings name
void check_library_folder(const std::filesystem::path& library) {
    for (const std::filesystem::path& name : library.relative_path()) {
        if (windows_name_problem(to_utf8(name)) != NameProblem::None) {
            throw std::runtime_error(std::format("Windows can't use \"{}\" as a folder name in the library folder", to_utf8(name)));
        }
    }
}

// The track's own file isn't among them: a file of its own may take that name
std::set<std::string> taken_names(const State& state, const std::string& track_key, const std::vector<std::filesystem::path>& in_folder) {
    std::set<std::string> taken;
    for (const auto& [key, track] : state.tracks) {
        if (key != track_key && !track.file.empty()) {
            taken.insert(file_name_key(to_utf8(track.file)));
        }
    }
    for (const std::filesystem::path& file : in_folder) {
        taken.insert(file_name_key(to_utf8(file.filename())));
    }
    if (auto track = state.tracks.find(track_key); track != state.tracks.end() && !track->second.file.empty()) {
        taken.erase(file_name_key(to_utf8(track->second.file)));
    }
    return taken;
}

std::string extension_of(const std::filesystem::path& file) {
    return to_utf8(file.extension()).substr(1);
}

bool same_name(std::string_view name, const std::filesystem::path& file) {
    return file_name_key(name) == file_name_key(to_utf8(file));
}

}

Library::Library(StateStore& state, platform::FileOps& files, const SettingsStore& settings, Log& log) :
        state_(state), files_(files), settings_(settings), log_(log) {}

std::shared_ptr<const State> Library::snapshot() const noexcept {
    return state_.snapshot();
}

std::optional<std::filesystem::path> Library::place(const std::string& track_key, const std::string& ytm_id, const TaggedAudio& audio) {
    std::scoped_lock lock(mutex_);
    std::shared_ptr<const State> state = state_.snapshot();
    auto track = state->tracks.find(track_key);
    if (track == state->tracks.end()) {
        return std::nullopt;
    }
    if (std::optional<ResolvedYtmId> resolved = resolved_ytm_id(*state, track->second); !resolved || resolved->ytm_id != ytm_id) {
        return std::nullopt;
    }
    std::filesystem::path library = settings_.current().library_path;
    check_library_folder(library);
    files_.create_folders(library);
    std::filesystem::path file = move_in(*state, track_key, audio, library);
    try {
        auto change = [&](State& changed) { apply_download(changed, track_key, ytm_id, file, audio.info); };
        update_locked(change);
    } catch (...) {
        // A state that can't be saved doesn't know of the file, so the next try would add another
        if (file != track->second.file) {
            files_.remove_file(library / file);
        }
        throw;
    }
    return file;
}

std::filesystem::path Library::move_in(const State& state, const std::string& track_key, const TaggedAudio& audio, const std::filesystem::path& library) {
    const std::filesystem::path& own = state.tracks.at(track_key).file;
    std::set<std::string> taken = taken_names(state, track_key, files_.list_files(library));
    for (int tries = 0; tries < PlacementTries; ++tries) {
        std::string name = free_file_name(audio.stem, extension_of(audio.audio), taken);
        // Downloaded again under the name it had, so the new file takes the old one's place
        if (!own.empty() && same_name(name, own)) {
            files_.move_replacing(audio.audio, library / own);
            return own;
        }
        std::filesystem::path file = path_from_utf8(name);
        if (files_.move_new(audio.audio, library / file)) {
            return file;
        }
        // Another file took the name since the folder was looked at
        taken.insert(file_name_key(name));
    }
    throw std::runtime_error(std::format("No free name for \"{}\" in the library folder", audio.stem));
}

void Library::settle(const State& before, const State& after) {
    FileChanges changes = file_changes(before, after);
    if (changes.dropped.empty() && changes.retagged.empty()) {
        return;
    }
    std::filesystem::path library = settings_.current().library_path;
    for (const DroppedFile& dropped : changes.dropped) {
        drop(dropped, before, library);
    }
    for (const RetaggedFile& retagged : changes.retagged) {
        retag(retagged, after, library);
    }
}

// The state is saved already, so a file that stays only costs room; the change doesn't fail for it
void Library::drop(const DroppedFile& dropped, const State& before, const std::filesystem::path& library) {
    LogSubject subject = track_subject(before, dropped.track);
    try {
        files_.remove_file(library / dropped.file);
        log_.write(LogLevel::Info, std::format("Deleted \"{}\"", to_utf8(dropped.file)), std::move(subject));
    } catch (const std::exception& error) {
        log_.write(LogLevel::Warning, std::format("Couldn't delete \"{}\": {}", to_utf8(dropped.file), one_line(error.what())), std::move(subject));
    }
}

void Library::retag(const RetaggedFile& retagged, const State& after, const std::filesystem::path& library) {
    LogSubject subject = track_subject(after, retagged.track);
    try {
        update_tags(library / retagged.file, retagged.tags);
    } catch (const std::exception& error) {
        log_.write(LogLevel::Warning, std::format("Couldn't re-tag \"{}\": {}", to_utf8(retagged.file), one_line(error.what())), std::move(subject));
        return;
    }
    log_.write(LogLevel::Debug, std::format("Re-tagged \"{}\"", to_utf8(retagged.file)), subject);
    if (retagged.stem) {
        rename(retagged, subject, library);
    }
}

void Library::rename(const RetaggedFile& retagged, const LogSubject& subject, const std::filesystem::path& library) {
    try {
        std::set<std::string> taken = taken_names(*state_.snapshot(), retagged.track, files_.list_files(library));
        for (int tries = 0; tries < PlacementTries; ++tries) {
            std::string name = free_file_name(*retagged.stem, extension_of(retagged.file), taken);
            if (same_name(name, retagged.file)) {
                return;
            }
            std::filesystem::path file = path_from_utf8(name);
            if (!files_.move_new(library / retagged.file, library / file)) {
                taken.insert(file_name_key(name));
                continue;
            }
            try {
                // Only the name changes, so there is nothing more to settle
                state_.update([&](State& state) {
                    auto track = state.tracks.find(retagged.track);
                    if (track != state.tracks.end() && track->second.file == retagged.file) {
                        track->second.file = file;
                    }
                });
            } catch (...) {
                // The state still names the file as it was
                files_.move_new(library / file, library / retagged.file);
                throw;
            }
            log_.write(LogLevel::Info, std::format("Renamed to \"{}\"", name), subject);
            return;
        }
        throw std::runtime_error(std::format("No free name for \"{}\"", *retagged.stem));
    } catch (const std::exception& error) {
        log_.write(LogLevel::Warning, std::format("Couldn't rename \"{}\": {}", to_utf8(retagged.file), one_line(error.what())), subject);
    }
}

}
