#include "core/sources/LinkUrl.h"

#include <algorithm>
#include <array>
#include <ranges>

namespace westie::core {

namespace {

constexpr std::string_view Spaces = " \t\r\n";
constexpr std::array<std::string_view, 2> Schemes = {"https://", "http://"};

char to_lower(char c) noexcept {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

std::vector<std::string_view> split(std::string_view text, char separator) {
    std::vector<std::string_view> parts;
    for (auto part : std::views::split(text, separator)) {
        parts.emplace_back(part.begin(), part.end());
    }
    return parts;
}

}

std::string_view trim_spaces(std::string_view text) {
    std::size_t first = text.find_first_not_of(Spaces);
    if (first == std::string_view::npos) {
        return {};
    }
    return text.substr(first, text.find_last_not_of(Spaces) - first + 1);
}

bool starts_with_ignoring_case(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && std::ranges::equal(text.substr(0, prefix.size()), prefix, {}, to_lower, to_lower);
}

bool is_letter_or_digit(char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

LinkUrl split_link_url(std::string_view text) {
    for (std::string_view scheme : Schemes) {
        if (starts_with_ignoring_case(text, scheme)) {
            text.remove_prefix(scheme.size());
            break;
        }
    }
    text = text.substr(0, text.find('#'));

    std::size_t host_end = std::min(text.find_first_of("/?"), text.size());
    std::string_view rest = text.substr(host_end);
    std::size_t query_start = std::min(rest.find('?'), rest.size());

    LinkUrl url;
    url.host = text.substr(0, host_end) | std::views::transform(to_lower) | std::ranges::to<std::string>();
    for (std::string_view name : split(rest.substr(0, query_start), '/')) {
        if (!name.empty()) {
            url.path.push_back(name);
        }
    }
    url.query = rest.substr(std::min(query_start + 1, rest.size()));
    return url;
}

std::string_view query_value(std::string_view query, std::string_view name) {
    for (std::string_view parameter : split(query, '&')) {
        if (parameter.size() > name.size() && parameter.starts_with(name) && parameter[name.size()] == '=') {
            return parameter.substr(name.size() + 1);
        }
    }
    return {};
}

}
