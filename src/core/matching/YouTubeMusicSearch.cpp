#include "core/matching/YouTubeMusicSearch.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <format>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <glaze/glaze.hpp>

#include "core/json/JsonOptions.h"

namespace westie::core {

namespace {

// The web player's client; YouTube Music answers older versions too, until it retires them
constexpr std::string_view ClientName = "WEB_REMIX";
constexpr std::string_view ClientVersion = "1.20251020.01.00";
// English, so the separators and lengths read the same everywhere
constexpr std::string_view Language = "en";
// The Songs filter of the search page
constexpr std::string_view SongsFilter = "EgWKAQIIAWoMEA4QChADEAQQCRAF";
constexpr std::string_view SearchPath = "/youtubei/v1/search?prettyPrint=false";
constexpr std::string_view GroupSeparator = " • ";
constexpr std::array<std::string_view, 2> ArtistSeparators = {", ", " & "};
constexpr int SecondsPerMinute = 60;

struct ClientJson {
    std::string client_name{ClientName};
    std::string client_version{ClientVersion};
    std::string hl{Language};

    struct glaze {
        constexpr static auto value = glz::object("clientName", &ClientJson::client_name, "clientVersion", &ClientJson::client_version, "hl", &ClientJson::hl);
    };
};

struct ContextJson {
    ClientJson client{};
};

struct RequestJson {
    ContextJson context{};
    std::string query{};
    std::string params{SongsFilter};
};

// The parts of the answer the search uses
struct RunJson {
    std::string text{};
};

struct TextJson {
    std::vector<RunJson> runs{};
};

struct ColumnRendererJson {
    TextJson text{};
};

struct ColumnJson {
    ColumnRendererJson renderer{};

    struct glaze {
        constexpr static auto value = glz::object("musicResponsiveListItemFlexColumnRenderer", &ColumnJson::renderer);
    };
};

struct PlaylistItemDataJson {
    std::string video_id{};

    struct glaze {
        constexpr static auto value = glz::object("videoId", &PlaylistItemDataJson::video_id);
    };
};

struct ItemRendererJson {
    std::optional<PlaylistItemDataJson> playlist_item_data{};
    std::vector<ColumnJson> flex_columns{};

    struct glaze {
        constexpr static auto value = glz::object("playlistItemData", &ItemRendererJson::playlist_item_data, "flexColumns", &ItemRendererJson::flex_columns);
    };
};

struct ShelfItemJson {
    std::optional<ItemRendererJson> renderer{};

    struct glaze {
        constexpr static auto value = glz::object("musicResponsiveListItemRenderer", &ShelfItemJson::renderer);
    };
};

struct ShelfJson {
    std::vector<ShelfItemJson> contents{};
};

struct SectionJson {
    std::optional<ShelfJson> shelf{};

    struct glaze {
        constexpr static auto value = glz::object("musicShelfRenderer", &SectionJson::shelf);
    };
};

struct SectionListJson {
    std::vector<SectionJson> contents{};
};

struct TabContentJson {
    std::optional<SectionListJson> section_list{};

    struct glaze {
        constexpr static auto value = glz::object("sectionListRenderer", &TabContentJson::section_list);
    };
};

struct TabRendererJson {
    TabContentJson content{};
};

struct TabJson {
    TabRendererJson renderer{};

    struct glaze {
        constexpr static auto value = glz::object("tabRenderer", &TabJson::renderer);
    };
};

struct TabbedResultsJson {
    std::vector<TabJson> tabs{};
};

struct ContentsJson {
    std::optional<TabbedResultsJson> tabbed_results{};

    struct glaze {
        constexpr static auto value = glz::object("tabbedSearchResultsRenderer", &ContentsJson::tabbed_results);
    };
};

struct AnswerJson {
    ContentsJson contents{};
};

std::string joined(const std::vector<std::string_view>& texts) {
    std::string text;
    for (std::string_view part : texts) {
        text += part;
    }
    return text;
}

std::optional<int> number(std::string_view digits) {
    int value = 0;
    auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (digits.empty() || error != std::errc() || end != digits.data() + digits.size()) {
        return std::nullopt;
    }
    return value;
}

std::optional<std::chrono::milliseconds> duration_of(std::string_view text) {
    int seconds = 0;
    bool any = false;
    while (!text.empty()) {
        std::size_t colon = text.find(':');
        std::optional<int> part = number(text.substr(0, colon));
        if (!part) {
            return std::nullopt;
        }
        seconds = seconds * SecondsPerMinute + *part;
        any = true;
        text = colon == std::string_view::npos ? std::string_view() : text.substr(colon + 1);
    }
    return any ? std::optional(std::chrono::milliseconds(std::chrono::seconds(seconds))) : std::nullopt;
}

std::optional<SongResult> song_of(const ItemRendererJson& item) {
    if (!item.playlist_item_data || item.playlist_item_data->video_id.empty() || item.flex_columns.size() < 2) {
        return std::nullopt;
    }
    SongResult song{.video_id = item.playlist_item_data->video_id};
    for (const RunJson& run : item.flex_columns[0].renderer.text.runs) {
        song.title += run.text;
    }

    // The second column reads "<artists> • <album> • <length>", with the artists' separators as runs of their own
    std::vector<std::vector<std::string_view>> groups(1);
    for (const RunJson& run : item.flex_columns[1].renderer.text.runs) {
        if (run.text == GroupSeparator) {
            groups.emplace_back();
        } else {
            groups.back().push_back(run.text);
        }
    }
    for (std::string_view artist : groups.front()) {
        if (!std::ranges::contains(ArtistSeparators, artist)) {
            song.artists.emplace_back(artist);
        }
    }
    if (groups.size() > 1) {
        if (std::optional<std::chrono::milliseconds> duration = duration_of(joined(groups.back()))) {
            song.duration = *duration;
            groups.pop_back();
        }
    }
    if (groups.size() > 1) {
        song.album = joined(groups[1]);
    }
    return song;
}

}

std::vector<SongResult> search_youtube_music_songs(platform::HttpClient& http, std::string_view api_url, std::string_view query, const std::stop_token& stop) {
    glz::expected<std::string, glz::error_ctx> request = glz::write_json(RequestJson{.query = std::string(query)});
    if (!request) {
        throw std::runtime_error(std::format("Couldn't write the search for \"{}\": {}", query, glz::format_error(request.error())));
    }
    std::string body = http.post(std::format("{}{}", api_url, SearchPath), {{.name = "Content-Type", .value = "application/json"}}, *request, stop);

    AnswerJson answer;
    if (glz::error_ctx error = glz::read<JsonReadOptions>(answer, body)) {
        throw std::runtime_error(std::format("YouTube Music's answer doesn't read: {}", glz::format_error(error, body)));
    }
    std::vector<SongResult> songs;
    if (!answer.contents.tabbed_results || answer.contents.tabbed_results->tabs.empty()) {
        return songs;
    }
    const std::optional<SectionListJson>& sections = answer.contents.tabbed_results->tabs.front().renderer.content.section_list;
    if (!sections) {
        return songs;
    }
    for (const SectionJson& section : sections->contents) {
        if (!section.shelf) {
            continue;
        }
        for (const ShelfItemJson& item : section.shelf->contents) {
            if (item.renderer) {
                if (std::optional<SongResult> song = song_of(*item.renderer)) {
                    songs.push_back(std::move(*song));
                }
            }
        }
    }
    return songs;
}

}
