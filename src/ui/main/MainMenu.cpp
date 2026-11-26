#include "ui/main/MainMenu.h"

#include <array>
#include <span>

#include <windows.h>

#include "platform/win32/Win32Error.h"
#include "ui/commands/CommandMenu.h"
#include "ui/framework/PopupMenu.h"
#include "ui/tracks/TrackMenu.h"

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

constexpr std::array UpdateItems = {
    command_item(CommandId::UpdateAll, L"Update &all\tF5"),
    command_item(CommandId::UpdateSelectedPlaylist, L"Update &selected playlist"),
    framework::MenuSeparator,
    command_item(CommandId::CancelUpdate, L"&Cancel"),
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
    command_item(CommandId::RelocateLibrary, L"Locate library &folder…"),
    framework::MenuSeparator,
    command_item(CommandId::OpenLogFolder, L"Open &log folder"),
};

constexpr std::array HelpItems = {
    command_item(CommandId::About, L"&About WestieLib"),
};

}

platform::win32::UniqueMenu build_main_menu() {
    std::array popups = {
        Popup{L"&File", FileItems}, Popup{L"&Update", UpdateItems}, Popup{L"&Track", track_menu_items()},
        Popup{L"&View", ViewItems}, Popup{L"T&ools", ToolsItems},   Popup{L"&Help", HelpItems},
    };
    platform::win32::UniqueMenu menu_bar(platform::win32::check(CreateMenu(), "CreateMenu"));
    for (const Popup& popup : popups) {
        platform::win32::UniqueMenu items = framework::build_popup_menu(popup.items);
        platform::win32::check(AppendMenuW(menu_bar.get(), MF_POPUP, reinterpret_cast<UINT_PTR>(items.get()), popup.title), "AppendMenuW");
        // The menu bar owns it now
        items.release();
    }
    return menu_bar;
}

}
