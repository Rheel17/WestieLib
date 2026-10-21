#include <exception>

#include <windows.h>
#include <knownfolders.h>

#include "core/Settings.h"
#include "core/SettingsStore.h"
#include "platform/win32/ComApartment.h"
#include "platform/win32/KnownFolder.h"
#include "platform/win32/Win32FileOps.h"
#include "ui/FatalError.h"
#include "ui/framework/AcceleratorTable.h"
#include "ui/framework/CommonControls.h"
#include "ui/framework/MessageLoop.h"
#include "ui/main/MainAccelerators.h"
#include "ui/main/MainWindow.h"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int show_command) {
    try {
        westie::platform::win32::ComApartment com;
        westie::platform::win32::Win32FileOps files;
        westie::core::SettingsStore settings_store(
                files, westie::platform::win32::known_folder(FOLDERID_LocalAppData) / L"WestieLib" / L"settings.json",
                {.library_path = westie::platform::win32::known_folder(FOLDERID_Music) / L"WestieLib"}
        );
        // A settings file that doesn't read stops the app before it shows anything
        westie::core::Settings settings = settings_store.load();

        westie::ui::framework::init_common_controls();
        westie::ui::MainWindow window(settings_store, settings);
        westie::ui::framework::AcceleratorTable accelerators = westie::ui::build_main_accelerators();
        window.show(show_command);
        return westie::ui::framework::MessageLoop{}.run(window.hwnd(), accelerators);
    } catch (const std::exception& error) {
        westie::ui::show_fatal_error(error.what());
    } catch (...) {
        westie::ui::show_fatal_error("Unknown error.");
    }

    return 1;
}
