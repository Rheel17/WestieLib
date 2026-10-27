#include "ui/inspector/TrackCard.h"

#include <algorithm>
#include <ranges>
#include <utility>
#include <vector>

#include "platform/win32/Utf16.h"
#include "platform/win32/Win32Error.h"
#include "ui/inspector/CardLayout.h"
#include "ui/inspector/TrackStatusText.h"
#include "ui/tracks/StatusTabText.h"

namespace westie::ui {

namespace {

constexpr int LineDips = 16;
constexpr int TitleLineDips = 20;
constexpr int CoverDips = 144;
constexpr int ArtistsGapDips = 1;
constexpr int StatusGapDips = 7;
constexpr int ExplanationGapDips = 3;
constexpr int ReasonGapDips = 8;
constexpr int OutputGapDips = 4;
constexpr int FileGapDips = 7;
constexpr IconTextStyle StatusStyle{.gap_dips = 6, .line_dips = 16};

}

TrackCard::TrackCard(HWND parent, InspectorEvents events) :
        Panel(parent, L"Track card"),
        events_(std::move(events)),
        cover_(hwnd()),
        title_(hwnd(), L"", {.wrap = true}),
        artists_(hwnd(), L"", {.wrap = true}),
        album_(hwnd(), L"", {.color = framework::TextColor::Gray}),
        status_section_(hwnd(), L"Status"),
        status_(hwnd(), StatusStyle),
        explanation_(hwnd(), L"", {.color = framework::TextColor::Gray, .wrap = true}),
        reason_(hwnd(), L"", {.color = framework::TextColor::Error}),
        output_(hwnd()),
        source_section_(hwnd(), L"Source"),
        first_source_key_(hwnd(), L"", {.color = framework::TextColor::Gray}),
        first_source_(hwnd(), [this] { open_source(0); }),
        second_source_key_(hwnd(), L"", {.color = framework::TextColor::Gray}),
        second_source_(hwnd(), [this] { open_source(1); }),
        playlists_key_(hwnd(), L"Playlists", {.color = framework::TextColor::Gray}),
        playlists_(hwnd(), [this](std::size_t index) { select_playlist(index); }),
        no_playlists_(hwnd(), L"None", {.color = framework::TextColor::Gray}),
        key_key_(hwnd(), L"Key", {.color = framework::TextColor::Gray}),
        key_(hwnd(), L""),
        audio_(hwnd(), events_.on_open_url),
        file_section_(hwnd(), L"File"),
        file_(hwnd(), L"Not downloaded yet.", {.color = framework::TextColor::Gray}) {}

int TrackCard::height_for(int width) const {
    return arrange(width, nullptr);
}

void TrackCard::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    cover_.apply_dpi(dpi);
    title_.set_font(fonts.title().handle());
    status_.apply_dpi(dpi, fonts.semibold());
    for (SectionTitle* section : {&status_section_, &source_section_, &file_section_}) {
        section->set_font(fonts.semibold().handle());
    }
    for (framework::Label* label : {
             &artists_, &album_, &explanation_, &reason_, &first_source_key_, &second_source_key_, &playlists_key_, &no_playlists_, &key_key_, &key_,
             &file_,
         }) {
        label->set_font(fonts.regular().handle());
    }
    output_.set_font(fonts.code().handle());
    audio_.apply_dpi(fonts);
    first_source_.apply_dpi(dpi, fonts.regular());
    second_source_.apply_dpi(dpi, fonts.regular());
    playlists_.set_font(fonts.regular().handle());
    line_height_ = std::max(dpi.scale(LineDips), fonts.regular().line_height());
    title_line_height_ = std::max(dpi.scale(TitleLineDips), fonts.title().line_height());
    layout();
}

void TrackCard::set_contents(const TrackDetails& track) {
    title_.set_text(track.row.title);
    artists_.set_text(track.row.artists);
    album_.set_text(track.row.album.empty() ? L"Album is read when it downloads" : track.row.album);
    status_.set(status_icon(track.row.status), status_word(track.row.status));
    explanation_.set_text(status_explanation(track.row.status, track.failure));
    reason_.set_text(failure_reason(track.failure));
    output_.set_text(track.error);

    bool several = track.sources.size() > 1;
    source_section_.set_text(several ? L"Sources" : L"Source");
    first_source_key_.set_text(several ? L"Listed on" : L"Source");
    if (!track.sources.empty()) {
        first_source_.set_source(track.sources[0].type);
    }
    if (several) {
        second_source_.set_source(track.sources[1].type);
    }
    playlists_.set_links(track.playlists | std::views::transform(&TrackPlaylist::name) | std::ranges::to<std::vector>());
    key_.set_text(platform::win32::to_utf16(track.row.key));
    audio_.set_contents(track);
    track_ = track;
    layout();
}

LRESULT TrackCard::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE: layout(); return 0;
    default:      return Panel::handle_message(message, wparam, lparam);
    }
}

int TrackCard::arrange(int width, framework::DeferredPositions* positions) const {
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    CardLayout layout(dpi, width, line_height_, positions);
    layout.cover(cover_.hwnd(), dpi.scale(CoverDips));
    layout.block(title_.hwnd(), 0, std::max(title_line_height_, title_.height_for(width)));
    layout.block(artists_.hwnd(), dpi.scale(ArtistsGapDips), std::max(line_height_, artists_.height_for(width)));
    layout.block(album_.hwnd(), 0, line_height_);

    layout.section(status_section_.hwnd());
    layout.block(status_.hwnd(), dpi.scale(StatusGapDips), status_.height());
    if (track_.row.status == StatusTab::NeedsReview) {
        layout.hide(explanation_.hwnd());
    } else {
        layout.block(explanation_.hwnd(), dpi.scale(ExplanationGapDips), explanation_.height_for(width));
    }
    if (track_.failure != core::TrackFailure::None) {
        layout.block(reason_.hwnd(), dpi.scale(ReasonGapDips), line_height_);
        layout.block(output_.hwnd(), dpi.scale(OutputGapDips), output_.height_for(width));
    } else {
        layout.hide(reason_.hwnd());
        layout.hide(output_.hwnd());
    }

    layout.section(source_section_.hwnd());
    layout.row_if(!track_.sources.empty(), first_source_key_.hwnd(), first_source_.hwnd());
    layout.row_if(track_.sources.size() > 1, second_source_key_.hwnd(), second_source_.hwnd());
    bool listed = !track_.playlists.empty();
    layout.row(playlists_key_.hwnd(), listed ? playlists_.hwnd() : no_playlists_.hwnd());
    layout.hide(listed ? no_playlists_.hwnd() : playlists_.hwnd());
    layout.row(key_key_.hwnd(), key_.hwnd());

    audio_.arrange(layout);

    layout.section(file_section_.hwnd());
    layout.block(file_.hwnd(), dpi.scale(FileGapDips), line_height_);
    return layout.height();
}

void TrackCard::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::DeferredPositions positions;
    arrange(client.right, &positions);
    positions.apply();
}

void TrackCard::open_source(std::size_t index) const {
    if (index < track_.sources.size()) {
        events_.on_open_url(track_.sources[index].url);
    }
}

void TrackCard::select_playlist(std::size_t index) const {
    if (index < track_.playlists.size()) {
        events_.on_select_playlist(track_.playlists[index].key);
    }
}

}
