#include "core/sources/SpotifyPlaylistName.h"

#include <format>
#include <stdexcept>
#include <utility>

#include <glaze/glaze.hpp>

#include "core/json/JsonOptions.h"

namespace westie::core {

namespace {

constexpr std::string_view DataStart = R"(<script id="__NEXT_DATA__" type="application/json">)";
constexpr std::string_view DataEnd = "</script>";

// The parts of the embed page's data the look-up uses: props.pageProps.state.data.entity.name
struct EntityJson {
    std::string name{};
};

struct DataJson {
    std::optional<EntityJson> entity{};
};

struct StateJson {
    DataJson data{};
};

struct PagePropsJson {
    // The error page for a missing or private playlist has none
    std::optional<StateJson> state{};
};

struct PropsJson {
    PagePropsJson page_props{};

    // glaze looks for a member of this name
    struct glaze {
        constexpr static auto value = glz::object("pageProps", &PropsJson::page_props);
    };
};

struct NextDataJson {
    PropsJson props{};
};

std::string page_data(std::string_view page) {
    std::size_t start = page.find(DataStart);
    std::size_t end = start == std::string_view::npos ? start : page.find(DataEnd, start + DataStart.size());
    if (end == std::string_view::npos) {
        throw std::runtime_error("Spotify's page has no playlist data");
    }
    return std::string(page.substr(start + DataStart.size(), end - start - DataStart.size()));
}

}

std::optional<std::string> spotify_playlist_name(platform::HttpClient& http, std::string_view spotify_url, std::string_view id, const std::stop_token& stop) {
    std::string data = page_data(http.get(std::format("{}/embed/playlist/{}", spotify_url, id), stop));
    NextDataJson json;
    if (glz::error_ctx error = glz::read<JsonReadOptions>(json, data)) {
        throw std::runtime_error(std::format("Spotify's page data doesn't read: {}", glz::format_error(error, data)));
    }

    std::optional<StateJson>& state = json.props.page_props.state;
    if (!state || !state->data.entity) {
        return std::nullopt;
    }
    return std::move(state->data.entity->name);
}

}
