#include "ui/inspector/PlaylistCard.h"

#include <algorithm>
#include <cstddef>
#include <format>
#include <utility>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/PlaylistTypeText.h"
#include "ui/RelativeDayText.h"
#include "ui/SourceLogo.h"
#include "ui/inspector/CardLayout.h"

namespace westie::ui {

namespace {

constexpr int LineDips = 16;

}

PlaylistCard::PlaylistCard(HWND parent, std::function<void(CommandId)> on_command) :
        Panel(parent, L"Playlist card"),
        on_command_(std::move(on_command)),
        title_(hwnd(), CardTitleStyle),
        subtitle_(hwnd(), L"", {.color = framework::TextColor::Gray}),
        section_(hwnd(), L"Playlist"),
        link_key_(hwnd(), L"Link", {.color = framework::TextColor::Gray}),
        link_(hwnd(), [this](std::size_t) { on_command_(CommandId::OpenPlaylistPage); }),
        tracks_key_(hwnd(), L"Tracks", {.color = framework::TextColor::Gray}),
        tracks_(hwnd(), L""),
        synced_key_(hwnd(), L"Synced", {.color = framework::TextColor::Gray}),
        synced_(hwnd()),
        waiting_key_(hwnd(), L"Waiting", {.color = framework::TextColor::Gray}),
        waiting_(hwnd(), L""),
        failed_key_(hwnd(), L"Failed", {.color = framework::TextColor::Gray}),
        failed_(hwnd(), L"", {.color = framework::TextColor::Error}),
        review_key_(hwnd(), L"To review", {.color = framework::TextColor::Gray}),
        review_(hwnd(), L"", {.color = framework::TextColor::Warning}),
        shared_key_(hwnd(), L"Shared", {.color = framework::TextColor::Gray}),
        shared_(hwnd(), L""),
        last_sync_key_(hwnd(), L"Last sync", {.color = framework::TextColor::Gray}),
        last_sync_(hwnd(), L""),
        sync_(hwnd(), {.text = L"Sync playlist", .command = std::to_underlying(CommandId::SyncSelectedPlaylist), .icon = IDI_SYNCING}),
        remove_(hwnd(), {.text = L"Remove playlist…", .command = std::to_underlying(CommandId::RemovePlaylist)}) {}

int PlaylistCard::height_for(int width) const {
    return arrange(width, nullptr);
}

void PlaylistCard::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    title_.apply_dpi(dpi, fonts.title());
    section_.set_font(fonts.semibold().handle());
    for (framework::Label* label : {
             &subtitle_, &link_key_, &tracks_key_, &tracks_, &synced_key_, &waiting_key_, &waiting_, &failed_key_, &failed_, &review_key_, &review_,
             &shared_key_, &shared_, &last_sync_key_, &last_sync_,
         }) {
        label->set_font(fonts.regular().handle());
    }
    link_.set_font(fonts.regular().handle());
    synced_.set_font(fonts.regular().handle());
    sync_.apply_dpi(dpi, fonts.regular());
    remove_.apply_dpi(dpi, fonts.regular());
    line_height_ = std::max(dpi.scale(LineDips), fonts.regular().line_height());
    layout();
}

void PlaylistCard::set_contents(const PlaylistRow& playlist) {
    title_.set(source_logo_icon(playlist.type), playlist.name);
    subtitle_.set_text(playlist_type_text(playlist.type));
    link_.set_text(playlist.id);
    tracks_.set_text(std::format(L"{}", playlist.counts.total));
    synced_.set_counts(playlist.counts);
    waiting_.set_text(std::format(L"{}", playlist.counts.waiting));
    failed_.set_text(std::format(L"{}", playlist.counts.failed));
    review_.set_text(std::format(L"{}", playlist.counts.to_review));
    const wchar_t* tracks = playlist.shared == 1 ? L"track is" : L"tracks are";
    shared_.set_text(std::format(L"{} {} also in other playlists", playlist.shared, tracks));
    last_sync_.set_text(playlist.last_sync ? relative_day_text(*playlist.last_sync, std::chrono::system_clock::now()) : L"Never");
    counts_ = playlist.counts;
    shared_count_ = playlist.shared;
    layout();
}

void PlaylistCard::update(const CommandState& state) {
    sync_.set_enabled(state.enabled(CommandId::SyncSelectedPlaylist));
    remove_.set_enabled(state.enabled(CommandId::RemovePlaylist));
}

LRESULT PlaylistCard::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:    layout(); return 0;
    case WM_COMMAND: on_command(wparam); return 0;
    default:         return Panel::handle_message(message, wparam, lparam);
    }
}

int PlaylistCard::arrange(int width, framework::DeferredPositions* positions) const {
    CardLayout layout(framework::Dpi::of(hwnd()), width, line_height_, positions);
    layout.title(title_.hwnd(), title_.height());
    layout.subtitle(subtitle_.hwnd(), title_.text_left());
    layout.section(section_.hwnd());
    layout.row(link_key_.hwnd(), link_.hwnd());
    layout.row(tracks_key_.hwnd(), tracks_.hwnd());
    layout.row(synced_key_.hwnd(), synced_.hwnd());
    layout.row_if(counts_.waiting > 0, waiting_key_.hwnd(), waiting_.hwnd());
    layout.row_if(counts_.failed > 0, failed_key_.hwnd(), failed_.hwnd());
    layout.row_if(counts_.to_review > 0, review_key_.hwnd(), review_.hwnd());
    layout.row_if(shared_count_ > 0, shared_key_.hwnd(), shared_.hwnd());
    layout.row(last_sync_key_.hwnd(), last_sync_.hwnd());
    layout.buttons({&sync_, &remove_});
    return layout.height();
}

void PlaylistCard::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::DeferredPositions positions;
    arrange(client.right, &positions);
    positions.apply();
}

void PlaylistCard::on_command(WPARAM wparam) const {
    if (HIWORD(wparam) == BN_CLICKED) {
        on_command_(static_cast<CommandId>(LOWORD(wparam)));
    }
}

}
