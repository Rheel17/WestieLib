#include "core/sources/SpotifyEmbedPage.h"

#include <format>
#include <stdexcept>
#include <utility>

#include <glaze/glaze.hpp>

#include "core/json/JsonOptions.h"

namespace westie::core {

namespace {

constexpr std::string_view DataStart = R"(<script id="__NEXT_DATA__" type="application/json">)";
constexpr std::string_view DataEnd = "</script>";

// The parts of the embed page's data the app uses: props.pageProps.state.data.entity.name and
// props.pageProps.state.settings.session.accessToken
struct EntityJson {
    std::string name{};
};

struct DataJson {
    std::optional<EntityJson> entity{};
};

struct SessionJson {
    std::string access_token{};

    struct glaze {
        constexpr static auto value = glz::object("accessToken", &SessionJson::access_token);
    };
};

struct SettingsJson {
    SessionJson session{};
};

struct StateJson {
    DataJson data{};
    SettingsJson settings{};
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

std::optional<SpotifyEmbedPage> read_spotify_embed_page(platform::HttpClient& http, std::string_view spotify_url, std::string_view id, const std::stop_token& stop) {
    std::string data = page_data(http.get(std::format("{}/embed/playlist/{}", spotify_url, id), {}, stop));
    NextDataJson json;
    if (glz::error_ctx error = glz::read<JsonReadOptions>(json, data)) {
        throw std::runtime_error(std::format("Spotify's page data doesn't read: {}", glz::format_error(error, data)));
    }

    std::optional<StateJson>& state = json.props.page_props.state;
    if (!state || !state->data.entity) {
        return std::nullopt;
    }
    return SpotifyEmbedPage{.name = std::move(state->data.entity->name), .access_token = std::move(state->settings.session.access_token)};
}

}
