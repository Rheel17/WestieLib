#include "core/sync/TrackDownload.h"

#include <exception>
#include <format>
#include <utility>

#include "core/library/LibraryFileName.h"
#include "core/log/LogLevel.h"
#include "core/log/LogSubject.h"
#include "core/log/LogText.h"
#include "core/tagging/Tagger.h"
#include "platform/Cancelled.h"

namespace westie::core {

TrackDownload::TrackDownload(const Downloader& downloader, const CoverArt& cover_art, platform::FileOps& files, Log& log) :
        downloader_(downloader), cover_art_(cover_art), files_(files), log_(log) {}

TaggedAudio TrackDownload::run(
        const State& state, const std::string& track_key, const std::string& ytm_id, const Settings& settings, const std::filesystem::path& work,
        const std::function<void(int percent)>& on_percent, const std::stop_token& stop
) const {
    const Track& track = state.tracks.at(track_key);
    LogSubject subject = track_subject(state, track_key);
    files_.create_folders(work);
    auto show_percent = [&](int percent) {
        on_percent(percent);
        log_.write(LogLevel::Debug, std::format("Downloaded {}%", percent), subject);
    };
    DownloadedAudio audio = downloader_.download(ytm_id, settings.format, work, subject, show_percent, stop);

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
                .cover = settings.embed_cover ? cover(metadata, audio, work, subject, stop) : std::string(),
            }
    );
    return {.audio = audio.audio, .stem = library_file_stem(metadata, ytm_id), .info = std::move(audio.info)};
}

// A file without a cover is better than no file
std::string TrackDownload::cover(
        const TrackMetadata& metadata, const DownloadedAudio& audio, const std::filesystem::path& work, const LogSubject& subject, const std::stop_token& stop
) const {
    try {
        return cover_art_.fetch(metadata.cover_url, audio.thumbnail, work, subject, stop);
    } catch (const platform::Cancelled&) {
        throw;
    } catch (const std::exception& error) {
        log_.write(LogLevel::Warning, std::format("Couldn't get the cover: {}", one_line(error.what())), subject);
        return {};
    }
}

}
