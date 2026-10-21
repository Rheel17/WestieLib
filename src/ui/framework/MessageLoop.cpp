#include "ui/framework/MessageLoop.h"

#include <windows.h>

#include "platform/win32/Win32Error.h"
#include "ui/framework/AcceleratorTable.h"
#include "ui/framework/PendingException.h"

namespace westie::ui::framework {

int MessageLoop::run(HWND window, const AcceleratorTable& accelerators) {
    MSG message;
    while (true) {
        BOOL result = GetMessageW(&message, nullptr, 0, 0);
        if (result == -1) {
            platform::win32::throw_last_error("GetMessageW");
        }
        if (result == 0) {
            return static_cast<int>(message.wParam);
        }

        if (!accelerators.translate(window, message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        PendingException::rethrow_if_any();
    }
}

}
