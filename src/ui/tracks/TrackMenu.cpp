#include "ui/tracks/TrackMenu.h"

#include <array>

#include "ui/commands/CommandMenu.h"

namespace westie::ui {

namespace {

constexpr std::array TrackItems = {
    command_item(CommandId::ConfirmMatch, L"&Confirm match"),
    command_item(CommandId::ChooseMatch, L"C&hoose match…"),
    command_item(CommandId::EditTrack, L"&Edit track…\tEnter"),
    framework::MenuSeparator,
    command_item(CommandId::Retry, L"&Retry"),
    command_item(CommandId::Keep, L"&Keep"),
    command_item(CommandId::DeleteFiles, L"&Delete file…"),
    framework::MenuSeparator,
    command_item(CommandId::ShowInFolder, L"&Show in folder"),
    command_item(CommandId::ShowInLog, L"Show in &log"),
    command_item(CommandId::OpenOnYouTubeMusic, L"Open on &YouTube Music"),
    command_item(CommandId::OpenOnSpotify, L"Open on S&potify"),
};

}

std::span<const framework::MenuItem> track_menu_items() {
    return TrackItems;
}

std::optional<CommandId> choose_track_command(HWND owner, POINT screen, const CommandState& state) {
    return choose_command(owner, screen, TrackItems, state);
}

}
