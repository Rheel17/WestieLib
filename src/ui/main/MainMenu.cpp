#include "ui/main/MainMenu.h"

#include <array>

#include <windows.h>

#include "platform/win32/Win32Error.h"

namespace westie::ui {

namespace {

constexpr std::array MenuTitles = {L"&File", L"&Sync", L"&Track", L"&View", L"&Tools", L"&Help"};

}

platform::win32::UniqueMenu build_main_menu() {
    platform::win32::UniqueMenu menu_bar(platform::win32::check(CreateMenu(), "CreateMenu"));
    for (const wchar_t* title : MenuTitles) {
        platform::win32::UniqueMenu popup(platform::win32::check(CreatePopupMenu(), "CreatePopupMenu"));
        platform::win32::check(AppendMenuW(menu_bar.get(), MF_POPUP, reinterpret_cast<UINT_PTR>(popup.get()), title), "AppendMenuW");
        // The menu bar owns it now
        popup.release();
    }
    return menu_bar;
}

}
