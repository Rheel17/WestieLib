#include "ui/inspector/AudioRows.h"

#include <cmath>
#include <cstddef>
#include <format>
#include <utility>

namespace westie::ui {

namespace {

constexpr double PercentPerWhole = 100.0;

framework::Label key_label(HWND card, const wchar_t* text) {
    return framework::Label(card, text, {.color = framework::TextColor::Gray});
}

const wchar_t* chooser_text(AudioChooser chooser) {
    switch (chooser) {
    case AudioChooser::Matcher:  return L"Matcher";
    case AudioChooser::Override: return L"Override";
    default:                     return L"Playlist";
    }
}

}

AudioRows::AudioRows(HWND card, std::function<void(const std::wstring&)> on_open_url) :
        on_open_url_(std::move(on_open_url)),
        section_(card, L"Audio"),
        matched_key_(key_label(card, L"Matched")),
        matched_(card, L""),
        artists_key_(key_label(card, L"Artists")),
        artists_(card, L""),
        length_key_(key_label(card, L"Length")),
        length_(card),
        id_key_(key_label(card, L"ID")),
        id_(card, [this](std::size_t) { on_open_url_(id_url_); }),
        no_id_(card, L"", {.color = framework::TextColor::Gray}),
        chosen_key_(key_label(card, L"Chosen by")),
        chosen_(card, L""),
        confidence_key_(key_label(card, L"Confidence")),
        confidence_(card),
        spotify_match_key_(key_label(card, L"Spotify match")),
        spotify_match_(card, L"") {}

void AudioRows::apply_dpi(const framework::FontSet& fonts) {
    section_.set_font(fonts.semibold().handle());
    for (framework::Label* label : {
             &matched_key_, &matched_, &artists_key_, &artists_, &length_key_, &id_key_, &no_id_, &chosen_key_, &chosen_, &confidence_key_,
             &spotify_match_key_, &spotify_match_,
         }) {
        label->set_font(fonts.regular().handle());
    }
    length_.set_font(fonts.regular().handle());
    id_.set_font(fonts.regular().handle());
    confidence_.set_font(fonts.regular().handle());
}

void AudioRows::set_contents(const TrackDetails& track) {
    has_id_ = track.audio.has_value();
    matcher_rows_ = has_id_ && track.audio->chosen_by == AudioChooser::Matcher && track.match;
    spotify_match_row_ = has_id_ && track.audio->chosen_by == AudioChooser::Playlist && track.match && track.match->ytm_id == track.audio->id;

    if (has_id_) {
        id_url_ = track.audio->url;
        id_.set_text(track.audio->id);
        chosen_.set_text(chooser_text(track.audio->chosen_by));
    } else {
        bool no_match = track.failure == core::TrackFailure::NoMatch;
        no_id_.set_text(no_match ? L"No match found" : L"Not chosen yet");
        no_id_.set_color(no_match ? framework::TextColor::Error : framework::TextColor::Gray);
    }
    if (track.match) {
        matched_.set_text(track.match->title);
        artists_.set_text(track.match->artists);
        length_.set_lengths(track.match->duration, track.match->source_duration);
        confidence_.set_confidence(track.match->confidence, track.row.status == StatusTab::NeedsReview);
        spotify_match_.set_text(std::format(L"Same ID ({}%)", std::lround(track.match->confidence * PercentPerWhole)));
    }
}

void AudioRows::arrange(CardLayout& layout) const {
    layout.section(section_.hwnd());
    layout.row_if(matcher_rows_, matched_key_.hwnd(), matched_.hwnd());
    layout.row_if(matcher_rows_, artists_key_.hwnd(), artists_.hwnd());
    layout.row_if(matcher_rows_, length_key_.hwnd(), length_.hwnd());
    layout.row(id_key_.hwnd(), has_id_ ? id_.hwnd() : no_id_.hwnd());
    layout.hide(has_id_ ? no_id_.hwnd() : id_.hwnd());
    layout.row_if(has_id_, chosen_key_.hwnd(), chosen_.hwnd());
    layout.row_if(matcher_rows_, confidence_key_.hwnd(), confidence_.hwnd());
    layout.row_if(spotify_match_row_, spotify_match_key_.hwnd(), spotify_match_.hwnd());
}

}
