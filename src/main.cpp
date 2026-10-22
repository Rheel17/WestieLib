#include <exception>
#include <filesystem>
#include <optional>
#include <string>

#include <windows.h>
#include <knownfolders.h>

#include "Version.h"
#include "core/settings/Settings.h"
#include "core/settings/SettingsStore.h"
#include "core/tools/ToolManager.h"
#include "platform/win32/BCryptHasher.h"
#include "platform/win32/ComApartment.h"
#include "platform/win32/Environment.h"
#include "platform/win32/KnownFolder.h"
#include "platform/win32/Utf16.h"
#include "platform/win32/Win32FileOps.h"
#include "platform/win32/WinHttpClient.h"
#include "ui/FatalError.h"
#include "ui/events/CoreEventQueue.h"
#include "ui/framework/AcceleratorTable.h"
#include "ui/framework/CommonControls.h"
#include "ui/framework/MessageLoop.h"
#include "ui/main/MainAccelerators.h"
#include "ui/main/MainWindow.h"

namespace {

// The UI checks point the app at a folder and a GitHub of their own, so they never touch the user's files or the network
std::filesystem::path data_folder() {
    if (std::optional<std::wstring> folder = westie::platform::win32::environment_variable(L"WESTIELIB_DATA_DIR"); folder && !folder->empty()) {
        return *folder;
    }
    return westie::platform::win32::known_folder(FOLDERID_LocalAppData) / L"WestieLib";
}

std::string github_api_url() {
    if (std::optional<std::wstring> url = westie::platform::win32::environment_variable(L"WESTIELIB_GITHUB_API"); url && !url->empty()) {
        return westie::platform::win32::to_utf8(*url);
    }
    return "https://api.github.com";
}

}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int show_command) {
    try {
        westie::platform::win32::ComApartment com;
        westie::platform::win32::Win32FileOps files;
        westie::platform::win32::WinHttpClient http(L"WestieLib/" WESTIELIB_VERSION_STRING);
        westie::platform::win32::BCryptHasher hasher;
        std::filesystem::path data = data_folder();
        westie::core::SettingsStore settings_store(
                files, data / L"settings.json", {.library_path = westie::platform::win32::known_folder(FOLDERID_Music) / L"WestieLib"}
        );
        // A settings file that doesn't read stops the app before it shows anything
        westie::core::Settings settings = settings_store.load();

        // The window goes before the tool manager, whose worker may still report into the queue while it stops
        westie::ui::CoreEventQueue events;
        westie::core::ToolManager tools({.files = files, .http = http, .hasher = hasher}, github_api_url(), data / L"tools", events);

        westie::ui::framework::init_common_controls();
        westie::ui::MainWindow window(settings_store, settings, events, tools);
        westie::ui::framework::AcceleratorTable accelerators = westie::ui::build_main_accelerators();
        // Started before the window shows, so its first paint already knows of the run
        tools.run(settings.auto_update_tools);
        window.show(show_command);
        return westie::ui::framework::MessageLoop{}.run(window.hwnd(), accelerators);
    } catch (const std::exception& error) {
        westie::ui::show_fatal_error(error.what());
    } catch (...) {
        westie::ui::show_fatal_error("Unknown error.");
    }

    return 1;
}
