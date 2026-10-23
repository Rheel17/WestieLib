#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <windows.h>
#include <knownfolders.h>

#include "Version.h"
#include "core/playlists/PlaylistLookup.h"
#include "core/settings/Settings.h"
#include "core/settings/SettingsStore.h"
#include "core/sources/SpotifyUrls.h"
#include "core/state/StateStore.h"
#include "core/sync/SyncEngine.h"
#include "core/tools/ToolFolder.h"
#include "core/tools/ToolManager.h"
#include "core/tools/YtDlp.h"
#include "platform/win32/BCryptHasher.h"
#include "platform/win32/ComApartment.h"
#include "platform/win32/Environment.h"
#include "platform/win32/KnownFolder.h"
#include "platform/win32/MinizArchiveExtractor.h"
#include "platform/win32/Utf16.h"
#include "platform/win32/Win32FileOps.h"
#include "platform/win32/Win32ProcessRunner.h"
#include "platform/win32/WinHttpClient.h"
#include "ui/FatalError.h"
#include "ui/events/CoreEventQueue.h"
#include "ui/framework/AcceleratorTable.h"
#include "ui/framework/CommonControls.h"
#include "ui/framework/GdiPlusSession.h"
#include "ui/framework/MessageLoop.h"
#include "ui/main/MainAccelerators.h"
#include "ui/main/MainWindow.h"

namespace {

// The UI checks point the app at a folder, a GitHub and a Spotify of their own, so they never touch the user's files or
// the network
std::filesystem::path data_folder() {
    if (std::optional<std::wstring> folder = westie::platform::win32::environment_variable(L"WESTIELIB_DATA_DIR"); folder && !folder->empty()) {
        return *folder;
    }
    return westie::platform::win32::known_folder(FOLDERID_LocalAppData) / L"WestieLib";
}

std::string service_url(const wchar_t* variable, std::string_view url) {
    if (std::optional<std::wstring> replaced = westie::platform::win32::environment_variable(variable); replaced && !replaced->empty()) {
        return westie::platform::win32::to_utf8(*replaced);
    }
    return std::string(url);
}

}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int show_command) {
    try {
        westie::platform::win32::ComApartment com;
        westie::platform::win32::Win32FileOps files;
        westie::platform::win32::WinHttpClient http(L"WestieLib/" WESTIELIB_VERSION_STRING);
        westie::platform::win32::BCryptHasher hasher;
        westie::platform::win32::MinizArchiveExtractor archives;
        westie::platform::win32::Win32ProcessRunner processes;
        std::filesystem::path data = data_folder();
        westie::core::SettingsStore settings_store(
                files, data / L"settings.json", {.library_path = westie::platform::win32::known_folder(FOLDERID_Music) / L"WestieLib"}
        );
        // A settings file that doesn't read stops the app before it shows anything
        westie::core::Settings settings = settings_store.load();
        // So does a state file that doesn't read
        westie::core::StateStore state(files, data / L"state.json");

        // The window goes before the tool manager, the look-up and the sync, whose workers may still report into the queue
        // while they stop
        westie::ui::CoreEventQueue events;
        westie::core::ToolFolder tool_folder(data / L"tools");
        westie::core::ToolManager tools(
                {.files = files, .http = http, .hasher = hasher, .archives = archives}, service_url(L"WESTIELIB_GITHUB_API", "https://api.github.com"),
                tool_folder, events
        );
        westie::core::YtDlp yt_dlp(tool_folder, files, processes);
        westie::core::SpotifyUrls spotify{
            .web = service_url(L"WESTIELIB_SPOTIFY_URL", "https://open.spotify.com"),
            .api = service_url(L"WESTIELIB_SPOTIFY_API", "https://api-partner.spotify.com"),
        };
        westie::core::PlaylistLookup lookup(yt_dlp, http, spotify.web, events);
        westie::core::SyncEngine sync(yt_dlp, tool_folder, http, spotify, state, events);

        westie::ui::framework::init_common_controls();
        westie::ui::framework::GdiPlusSession gdi_plus;
        westie::ui::MainWindow window(settings_store, settings, state, events, tools, lookup, sync);
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
