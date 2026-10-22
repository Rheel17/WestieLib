#include "core/playlists/PlaylistLookup.h"

#include <exception>
#include <utility>

#include "core/playlists/PlaylistLookedUp.h"
#include "core/sources/SpotifyPlaylistName.h"
#include "core/sources/YouTubeMusicPlaylistName.h"
#include "core/tools/ToolMissing.h"
#include "platform/Cancelled.h"

namespace westie::core {

PlaylistLookup::PlaylistLookup(const YtDlp& yt_dlp, platform::HttpClient& http, std::string spotify_url, EventSink& events) :
        yt_dlp_(yt_dlp), http_(http), spotify_url_(std::move(spotify_url)), events_(events) {}

void PlaylistLookup::look_up(PlaylistLink link) {
    // Assigning stops the look-up before this one and waits for it to end
    worker_ = std::jthread([this, link = std::move(link)](std::stop_token stop) { work(stop, link); });
}

void PlaylistLookup::stop() noexcept {
    worker_.request_stop();
}

void PlaylistLookup::work(const std::stop_token& stop, const PlaylistLink& link) {
    PlaylistLookedUp answer{.key = link.key()};
    // An exception that leaves a thread ends the process, so it becomes the answer instead
    try {
        std::optional<std::string> found = name(link, stop);
        answer.outcome = found ? PlaylistLookupOutcome::Found : PlaylistLookupOutcome::Unavailable;
        answer.name = std::move(found).value_or("");
    } catch (const platform::Cancelled&) {
        return;
    } catch (const ToolMissing&) {
        answer.outcome = PlaylistLookupOutcome::ToolMissing;
    } catch (const std::exception& error) {
        answer.outcome = PlaylistLookupOutcome::Failed;
        answer.error = error.what();
    }
    // Only a newer look-up or the dialog's close stops one, and neither waits for its answer
    if (!stop.stop_requested()) {
        events_.emit(std::move(answer));
    }
}

std::optional<std::string> PlaylistLookup::name(const PlaylistLink& link, const std::stop_token& stop) const {
    switch (link.type) {
    case SourceType::YouTubeMusic: return youtube_music_playlist_name(yt_dlp_, link, stop);
    case SourceType::Spotify:      return spotify_playlist_name(http_, spotify_url_, link.id, stop);
    }
    return std::nullopt;
}

}
