#include "core/sync/TrackDownload.h"

#include <exception>
#include <format>
#include <set>
#include <stdexcept>
#include <utility>

#include "core/json/Utf8Path.h"
#include "core/library/LibraryFileName.h"
#include "core/library/WindowsNames.h"
#include "core/log/LogLevel.h"
#include "core/log/LogSubject.h"
#include "core/log/LogText.h"
#include "core/tagging/Tagger.h"
#include "platform/Cancelled.h"

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
    return taken;
}

}

TrackDownload::TrackDownload(const Downloader& downloader, const CoverArt& cover_art, platform::FileOps& files, Log& log) :
        downloader_(downloader), cover_art_(cover_art), files_(files), log_(log) {}

PlacedTrack TrackDownload::run(
        const State& state, const std::string& track_key, const std::string& ytm_id, const Settings& settings, const std::filesystem::path& work,
        const std::function<void(int percent)>& on_percent, const std::stop_token& stop
) const {
    const Track& track = state.tracks.at(track_key);
    files_.create_folders(work);
    DownloadedAudio audio = downloader_.download(ytm_id, settings.format, work, on_percent, stop);

    // The video's info fills in what the sources don't know yet, as it will once it is stored
    TrackMetadata metadata = effective_metadata(state, track);
    if (metadata.album.empty()) {
        metadata.album = audio.info.album;
    }
    if (!metadata.year) {
        metadata.year = audio.info.year;
    }
    write_tags(
            audio.audio,
            {
                .title = metadata.title,
                .artists = metadata.artists,
                .album = metadata.album,
                .year = metadata.year,
                .sources = track.sources,
                .ytm_id = ytm_id,
                .cover = settings.embed_cover ? cover(state, track_key, metadata, audio, work, stop) : std::string(),
            }
    );
    return {.file = place(state, track_key, metadata, ytm_id, audio.audio, settings.library_path), .info = std::move(audio.info)};
}

// A file without a cover is better than no file
std::string TrackDownload::cover(
        const State& state, const std::string& track_key, const TrackMetadata& metadata, const DownloadedAudio& audio, const std::filesystem::path& work,
        const std::stop_token& stop
) const {
    try {
        return cover_art_.fetch(metadata.cover_url, audio.thumbnail, work, stop);
    } catch (const platform::Cancelled&) {
        throw;
    } catch (const std::exception& error) {
        log_.write(LogLevel::Warning, std::format("Couldn't get the cover: {}", one_line(error.what())), track_subject(state, track_key));
        return {};
    }
}

std::filesystem::path TrackDownload::place(
        const State& state, const std::string& track_key, const TrackMetadata& metadata, const std::string& ytm_id, const std::filesystem::path& audio,
        const std::filesystem::path& library
) const {
    check_library_folder(library);
    files_.create_folders(library);
    std::set<std::string> taken = taken_names(state, track_key, files_.list_files(library));
    std::string stem = library_file_stem(metadata, ytm_id);
    std::string extension = to_utf8(audio.extension()).substr(1);
    for (int tries = 0; tries < PlacementTries; ++tries) {
        std::string name = free_file_name(stem, extension, taken);
        std::filesystem::path file = path_from_utf8(name);
        if (files_.move_new(audio, library / file)) {
            return file;
        }
        // Another file took the name since the folder was looked at
        taken.insert(file_name_key(name));
    }
    throw std::runtime_error(std::format("No free name for \"{}\" in the library folder", stem));
}

}
