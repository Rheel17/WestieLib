#include "ui/inspector/DoubtList.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <string_view>

#include "ui/DurationText.h"

namespace westie::ui {

namespace {

constexpr std::array<core::MatchDoubt, 5> DoubtOrder = {
    core::MatchDoubt::Title, core::MatchDoubt::Artist, core::MatchDoubt::Version, core::MatchDoubt::Length, core::MatchDoubt::Explicit,
};
// A match from before the Matcher stored its doubts
constexpr std::wstring_view NoDoubts = L"Low confidence";
constexpr std::wstring_view Bullet = L"• ";

std::wstring version_line(const TrackMatch& match) {
    if (!match.video_words.empty() && !match.source_words.empty()) {
        return std::format(L"“{}” instead of “{}”", match.video_words, match.source_words);
    }
    if (!match.video_words.empty()) {
        return std::format(L"Adds “{}”", match.video_words);
    }
    if (!match.source_words.empty()) {
        return std::format(L"Lacks “{}”", match.source_words);
    }
    return L"";
}

std::wstring length_line(const TrackMatch& match) {
    if (match.duration.count() <= 0 || match.source_duration.count() <= 0) {
        return L"Length unknown";
    }
    // In whole seconds, as the Length row shows the two lengths
    std::chrono::seconds off =
            std::chrono::duration_cast<std::chrono::seconds>(match.duration) - std::chrono::duration_cast<std::chrono::seconds>(match.source_duration);
    return std::format(L"{} {}", duration_text(std::chrono::abs(off)), off.count() > 0 ? L"longer" : L"shorter");
}

std::wstring explicit_line(const TrackMatch& match) {
    if (!match.explicit_content) {
        return L"";
    }
    return *match.explicit_content ? L"Explicit version" : L"Clean version";
}

std::wstring doubt_line(const TrackMatch& match, core::MatchDoubt doubt) {
    switch (doubt) {
    case core::MatchDoubt::Title:    return L"Title differs slightly";
    case core::MatchDoubt::Artist:   return L"Other artists";
    case core::MatchDoubt::Version:  return version_line(match);
    case core::MatchDoubt::Length:   return length_line(match);
    case core::MatchDoubt::Explicit: return explicit_line(match);
    }
    return L"";
}

}

std::wstring doubt_list(const std::optional<TrackMatch>& match) {
    std::wstring list;
    for (core::MatchDoubt doubt : DoubtOrder) {
        if (match && std::ranges::contains(match->doubts, doubt)) {
            std::wstring line = doubt_line(*match, doubt);
            if (!line.empty()) {
                list += std::format(L"{}{}{}", list.empty() ? L"" : L"\n", Bullet, line);
            }
        }
    }
    return list.empty() ? std::format(L"{}{}", Bullet, NoDoubts) : list;
}

}
