#include "core/sources/SpotifyPlaylist.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <glaze/glaze.hpp>

#include "core/json/JsonOptions.h"
#include "core/sources/SourceType.h"
#include "core/sources/SpotifyEmbedPage.h"
#include "platform/Cancelled.h"
#include "platform/HttpError.h"

namespace westie::core {

namespace {

// Spotify changes the hash now and then; RemoteConfig will serve these once it exists
constexpr std::string_view OperationName = "fetchPlaylist";
constexpr std::string_view PersistedQueryHash = "a65e12194ed5fc443a1cdebed5fabe33ca5b07b987185d63c72483867ad13cb4";
constexpr int PageSize = 100;
constexpr int FirstServerErrorStatus = 500;
constexpr std::chrono::milliseconds RetryDelay{1000};

constexpr std::string_view PlaylistType = "Playlist";
constexpr std::string_view NotFoundType = "NotFound";
constexpr std::string_view TrackWrapperType = "TrackResponseWrapper";
constexpr std::string_view TrackType = "Track";
constexpr std::string_view TrackUriPrefix = "spotify:track:";
constexpr std::string_view UnavailableMessage = "The playlist doesn't exist or is private.";

// The parts of the fetchPlaylist answer the fetch uses
struct ProfileJson {
    std::string name{};
};

struct ArtistJson {
    ProfileJson profile{};
};

struct ArtistsJson {
    std::vector<ArtistJson> items{};
};

struct AlbumJson {
    std::string name{};
};

struct DurationJson {
    std::int64_t total_milliseconds = 0;

    struct glaze {
        constexpr static auto value = glz::object("totalMilliseconds", &DurationJson::total_milliseconds);
    };
};

struct TrackJson {
    std::string type_name{};
    std::string uri{};
    std::string name{};
    ArtistsJson artists{};
    std::optional<AlbumJson> album_of_track{};
    DurationJson track_duration{};

    struct glaze {
        using T = TrackJson;
        constexpr static auto value = glz::object(
                "__typename", &T::type_name, "uri", &T::uri, "name", &T::name, "artists", &T::artists, "albumOfTrack", &T::album_of_track, "trackDuration",
                &T::track_duration
        );
    };
};

// Episodes and local files hold other fields, so an item's data is read only once its type says it is a track
struct ItemV2Json {
    std::string type_name{};
    glz::raw_json data{};

    struct glaze {
        constexpr static auto value = glz::object("__typename", &ItemV2Json::type_name, "data", &ItemV2Json::data);
    };
};

struct ItemJson {
    ItemV2Json item_v2{};

    struct glaze {
        constexpr static auto value = glz::object("itemV2", &ItemJson::item_v2);
    };
};

struct ContentJson {
    std::int64_t total_count = 0;
    std::vector<ItemJson> items{};

    struct glaze {
        constexpr static auto value = glz::object("totalCount", &ContentJson::total_count, "items", &ContentJson::items);
    };
};

struct PlaylistJson {
    std::string type_name{};
    ContentJson content{};

    struct glaze {
        constexpr static auto value = glz::object("__typename", &PlaylistJson::type_name, "content", &PlaylistJson::content);
    };
};

struct DataJson {
    PlaylistJson playlist_v2{};

    struct glaze {
        constexpr static auto value = glz::object("playlistV2", &DataJson::playlist_v2);
    };
};

struct ErrorJson {
    std::string message{};
};

struct AnswerJson {
    std::optional<DataJson> data{};
    std::vector<ErrorJson> errors{};
};

bool is_unreserved(char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-' || c == '_' || c == '.' || c == '~';
}

std::string percent_encode(std::string_view text) {
    std::string encoded;
    for (char c : text) {
        if (is_unreserved(c)) {
            encoded += c;
        } else {
            encoded += std::format("%{:02X}", static_cast<unsigned char>(c));
        }
    }
    return encoded;
}

std::string query_url(std::string_view api_url, std::string_view id, std::int64_t offset) {
    std::string variables = std::format(R"({{"uri":"spotify:playlist:{}","offset":{},"limit":{},"enableWatchFeedEntrypoint":false}})", id, offset, PageSize);
    std::string extensions = std::format(R"({{"persistedQuery":{{"version":1,"sha256Hash":"{}"}}}})", PersistedQueryHash);
    return std::format(
            "{}/pathfinder/v1/query?operationName={}&variables={}&extensions={}", api_url, OperationName, percent_encode(variables), percent_encode(extensions)
    );
}

void pause(std::chrono::milliseconds delay, const std::stop_token& stop) {
    std::mutex mutex;
    std::condition_variable_any never_notified;
    std::unique_lock lock(mutex);
    never_notified.wait_for(lock, stop, delay, [] { return false; });
    if (stop.stop_requested()) {
        throw platform::Cancelled();
    }
}

std::string get_with_retry(platform::HttpClient& http, const std::string& url, const std::vector<platform::HttpHeader>& headers, const std::stop_token& stop) {
    try {
        return http.get(url, headers, stop);
    } catch (const platform::HttpError& error) {
        if (error.status() < FirstServerErrorStatus) {
            throw;
        }
    }
    // Spotify's servers fail a request now and then, and the same request goes through a moment later
    pause(RetryDelay, stop);
    return http.get(url, headers, stop);
}

ContentJson read_page(platform::HttpClient& http, const std::string& url, const std::vector<platform::HttpHeader>& headers, const std::stop_token& stop) {
    std::string body = get_with_retry(http, url, headers, stop);
    AnswerJson answer;
    if (glz::error_ctx error = glz::read<JsonReadOptions>(answer, body)) {
        throw std::runtime_error(std::format("Spotify's answer doesn't read: {}", glz::format_error(error, body)));
    }
    if (!answer.errors.empty()) {
        throw std::runtime_error(std::format("Spotify said: {}", answer.errors.front().message));
    }
    if (answer.data && answer.data->playlist_v2.type_name == NotFoundType) {
        throw std::runtime_error(std::string(UnavailableMessage));
    }
    if (!answer.data || answer.data->playlist_v2.type_name != PlaylistType) {
        throw std::runtime_error("Spotify's answer has no playlist");
    }
    return std::move(answer.data->playlist_v2.content);
}

std::optional<SourceTrack> source_track(const ItemJson& item) {
    if (item.item_v2.type_name != TrackWrapperType) {
        return std::nullopt;
    }
    TrackJson track;
    if (glz::error_ctx error = glz::read<JsonReadOptions>(track, item.item_v2.data.str)) {
        throw std::runtime_error(std::format("Spotify's answer doesn't read: {}", glz::format_error(error, item.item_v2.data.str)));
    }
    if (track.type_name != TrackType || !track.uri.starts_with(TrackUriPrefix)) {
        return std::nullopt;
    }

    std::vector<std::string> artists;
    for (ArtistJson& artist : track.artists.items) {
        artists.push_back(std::move(artist.profile.name));
    }
    return SourceTrack{
        .key = std::format("{}:{}", source_prefix(SourceType::Spotify), std::string_view(track.uri).substr(TrackUriPrefix.size())),
        .title = std::move(track.name),
        .artists = std::move(artists),
        .album = std::move(track.album_of_track).value_or(AlbumJson{}).name,
        .duration = std::chrono::milliseconds(track.track_duration.total_milliseconds),
    };
}

}

PlaylistContents fetch_spotify_playlist(
        platform::HttpClient& http, const SpotifyUrls& urls, const PlaylistLink& link, const std::function<void(const PlaylistContents& so_far)>& on_page,
        const std::stop_token& stop
) {
    std::optional<SpotifyEmbedPage> page = read_spotify_embed_page(http, urls.web, link.id, stop);
    if (!page) {
        throw std::runtime_error(std::string(UnavailableMessage));
    }
    if (page->access_token.empty()) {
        throw std::runtime_error("Spotify's page has no access token");
    }
    std::vector<platform::HttpHeader> headers = {
        {.name = "Authorization", .value = std::format("Bearer {}", page->access_token)},
        {.name = "app-platform", .value = "WebPlayer"},
    };

    PlaylistContents contents{.name = std::move(page->name)};
    std::int64_t offset = 0;
    while (true) {
        ContentJson content = read_page(http, query_url(urls.api, link.id, offset), headers, stop);
        for (const ItemJson& item : content.items) {
            if (std::optional<SourceTrack> track = source_track(item)) {
                contents.tracks.push_back(std::move(*track));
            }
        }
        offset += std::ssize(content.items);
        if (content.items.empty() || offset >= content.total_count) {
            return contents;
        }
        on_page(contents);
    }
}

}
