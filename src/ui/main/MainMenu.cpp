#include "ui/main/MainMenu.h"

#include <array>
#include <span>

#include <windows.h>

#include "platform/win32/Win32Error.h"
#include "ui/commands/CommandMenu.h"
#include "ui/framework/PopupMenu.h"

namespace westie::ui {

namespace {

struct Popup {
    const wchar_t* title = nullptr;
    std::span<const framework::MenuItem> items{};
};

constexpr std::array FileItems = {
    command_item(CommandId::AddPlaylist, L"&Add playlist…\tCtrl+N"),
    command_item(CommandId::RemovePlaylist, L"&Remove playlist…"),
    framework::MenuSeparator,
    command_item(CommandId::OpenLibraryFolder, L"&Open library folder"),
    framework::MenuSeparator,
    command_item(CommandId::Exit, L"E&xit\tCtrl+Q"),
};

constexpr std::array SyncItems = {
    command_item(CommandId::SyncAll, L"Sync &all\tF5"),
    command_item(CommandId::SyncSelectedPlaylist, L"Sync &selected playlist"),
    framework::MenuSeparator,
    command_item(CommandId::CancelSync, L"&Cancel"),
};

constexpr std::array TrackItems = {
    command_item(CommandId::ConfirmMatch, L"&Confirm match"),
    command_item(CommandId::ChooseVideo, L"C&hoose video…"),
    command_item(CommandId::EditTrack, L"&Edit track…\tEnter"),
    framework::MenuSeparator,
    command_item(CommandId::Retry, L"&Retry"),
    command_item(CommandId::Keep, L"&Keep"),
    command_item(CommandId::MoveToRecycleBin, L"&Move to Recycle Bin…"),
    framework::MenuSeparator,
    command_item(CommandId::ShowInFolder, L"&Show in folder"),
    command_item(CommandId::OpenOnYouTube, L"Open on &YouTube"),
    command_item(CommandId::OpenSourcePage, L"Open source &page"),
};

constexpr std::array ViewItems = {
    command_item(CommandId::ToggleLog, L"&Log\tCtrl+L"),
    framework::MenuSeparator,
    command_item(CommandId::Columns, L"&Columns…"),
};

constexpr std::array ToolsItems = {
    command_item(CommandId::Settings, L"&Settings…\tCtrl+Alt+S"),
    command_item(CommandId::CheckForUpdates, L"Check for &updates"),
    framework::MenuSeparator,
    command_item(CommandId::RebuildLibrary, L"&Rebuild library from files…"),
    command_item(CommandId::RelocateLibrary, L"Library folder &moved…"),
    framework::MenuSeparator,
    command_item(CommandId::OpenLogFolder, L"Open &log folder"),
};

constexpr std::array HelpItems = {
    command_item(CommandId::About, L"&About WestieLib"),
};

constexpr std::array Popups = {
    Popup{L"&File", FileItems}, Popup{L"&Sync", SyncItems},   Popup{L"&Track", TrackItems},
    Popup{L"&View", ViewItems}, Popup{L"T&ools", ToolsItems}, Popup{L"&Help", HelpItems},
};

}

platform::win32::UniqueMenu build_main_menu() {
    platform::win32::UniqueMenu menu_bar(platform::win32::check(CreateMenu(), "CreateMenu"));
    for (const Popup& popup : Popups) {
        platform::win32::UniqueMenu items = framework::build_popup_menu(popup.items);
        platform::win32::check(AppendMenuW(menu_bar.get(), MF_POPUP, reinterpret_cast<UINT_PTR>(items.get()), popup.title), "AppendMenuW");
        // The menu bar owns it now
        items.release();
    }
    return menu_bar;
}

}
