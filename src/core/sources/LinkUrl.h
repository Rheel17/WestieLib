#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace westie::core {

// The hosts whose pages a YouTube Music or YouTube link may name
constexpr std::array<std::string_view, 4> YouTubeHosts = {"music.youtube.com", "youtube.com", "www.youtube.com", "m.youtube.com"};

// A pasted link in parts. The views point into the text it was split from.
struct LinkUrl {
    // In lowercase
    std::string host{};
    // Without empty names: a doubled or trailing slash doesn't change the page
    std::vector<std::string_view> path{};
    std::string_view query{};
};

std::string_view trim_spaces(std::string_view text);
bool starts_with_ignoring_case(std::string_view text, std::string_view prefix);
bool is_letter_or_digit(char c) noexcept;
// With or without http:// or https://, and without a #fragment
LinkUrl split_link_url(std::string_view text);
// Empty when the query hasn't the parameter
std::string_view query_value(std::string_view query, std::string_view name);

}
