#include "ui/inspector/LibraryCard.h"

#include <algorithm>
#include <utility>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/TrackCountText.h"
#include "ui/inspector/CardLayout.h"

namespace westie::ui {

namespace {

constexpr int LineDips = 16;

}

LibraryCard::LibraryCard(HWND parent, std::function<void(CommandId)> on_command) :
        Panel(parent, L"Library card"),
        on_command_(std::move(on_command)),
        title_(hwnd(), CardTitleStyle),
        section_(hwnd(), L"Library"),
        folder_key_(hwnd(), L"Folder", {.color = framework::TextColor::Gray}),
        folder_(hwnd(), L"", {.ellipsis = framework::TextEllipsis::Path}),
        tracks_key_(hwnd(), L"Tracks", {.color = framework::TextColor::Gray}),
        tracks_(hwnd(), L""),
        open_(hwnd(), {.text = L"Open library folder", .command = std::to_underlying(CommandId::OpenLibraryFolder)}) {
    title_.set(IDI_LIBRARY, L"All tracks");
}

int LibraryCard::height_for(int width) const {
    return arrange(width, nullptr);
}

void LibraryCard::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    title_.apply_dpi(dpi, fonts.title());
    section_.set_font(fonts.semibold().handle());
    for (framework::Label* label : {&folder_key_, &folder_, &tracks_key_, &tracks_}) {
        label->set_font(fonts.regular().handle());
    }
    open_.apply_dpi(dpi, fonts.regular());
    line_height_ = std::max(dpi.scale(LineDips), fonts.regular().line_height());
    layout();
}

void LibraryCard::set_contents(const std::filesystem::path& folder, const TrackCounts& counts) {
    folder_.set_text(folder.wstring());
    tracks_.set_text(synced_count_text(counts));
}

void LibraryCard::update(const CommandState& state) {
    open_.set_enabled(state.enabled(CommandId::OpenLibraryFolder));
}

LRESULT LibraryCard::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:    layout(); return 0;
    case WM_COMMAND: on_command(wparam); return 0;
    default:         return Panel::handle_message(message, wparam, lparam);
    }
}

int LibraryCard::arrange(int width, framework::DeferredPositions* positions) const {
    CardLayout layout(framework::Dpi::of(hwnd()), width, line_height_, positions);
    layout.title(title_.hwnd(), title_.height());
    layout.section(section_.hwnd());
    layout.row(folder_key_.hwnd(), folder_.hwnd());
    layout.row(tracks_key_.hwnd(), tracks_.hwnd());
    layout.buttons({&open_});
    return layout.height();
}

void LibraryCard::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::DeferredPositions positions;
    arrange(client.right, &positions);
    positions.apply();
}

void LibraryCard::on_command(WPARAM wparam) const {
    if (HIWORD(wparam) == BN_CLICKED) {
        on_command_(static_cast<CommandId>(LOWORD(wparam)));
    }
}

}
