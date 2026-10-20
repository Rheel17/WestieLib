#include <exception>

#include <windows.h>

#include "ui/FatalError.h"
#include "ui/framework/MessageLoop.h"
#include "ui/main/MainWindow.h"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int show_command) {
    try {
        westie::ui::MainWindow window;
        window.show(show_command);
        return westie::ui::framework::MessageLoop{}.run();
    } catch (const std::exception& error) {
        westie::ui::show_fatal_error(error.what());
    } catch (...) {
        westie::ui::show_fatal_error("Unknown error.");
    }

    return 1;
}
