#include "core/matching/MatchText.h"

#include <cmath>
#include <format>
#include <string_view>

namespace westie::core {

namespace {

constexpr double PercentPerWhole = 100.0;

std::string_view doubt_word(MatchDoubt doubt) noexcept {
    switch (doubt) {
    case MatchDoubt::Title:    return "title";
    case MatchDoubt::Artist:   return "artist";
    case MatchDoubt::Version:  return "version";
    case MatchDoubt::Length:   return "length";
    case MatchDoubt::Explicit: return "explicit";
    }
    return "";
}

std::string joined(const std::vector<std::string>& parts) {
    std::string text;
    for (const std::string& part : parts) {
        if (!text.empty()) {
            text += ", ";
        }
        text += part;
    }
    return text;
}

}

std::string result_text(const std::string& title, const std::vector<std::string>& artists, double confidence) {
    std::string text = std::format("\"{}\"", title);
    if (!artists.empty()) {
        text += std::format(" by {}", joined(artists));
    }
    return text + std::format(" ({}%)", std::lround(confidence * PercentPerWhole));
}

std::string match_text(const Match& match) {
    return result_text(match.title, match.artists, match.confidence);
}

std::string doubts_text(const std::vector<MatchDoubt>& doubts) {
    std::string text;
    for (MatchDoubt doubt : doubts) {
        text += text.empty() ? "doubts: " : ", ";
        text += doubt_word(doubt);
    }
    return text;
}

}
