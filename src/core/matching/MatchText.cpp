#include "core/matching/MatchText.h"

#include <cmath>
#include <format>
#include <string_view>

namespace westie::core {

namespace {

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

std::string match_text(const Match& match) {
    std::string text = std::format("\"{}\"", match.title);
    if (!match.artists.empty()) {
        text += std::format(" by {}", joined(match.artists));
    }
    return text + std::format(" ({}%)", std::lround(match.confidence * 100));
}

std::string doubts_text(const std::vector<MatchDoubt>& doubts) {
    std::string text;
    for (MatchDoubt doubt : doubts) {
        text += text.empty() ? "Check " : ", ";
        text += doubt_word(doubt);
    }
    return text.empty() ? text : text + '.';
}

}
