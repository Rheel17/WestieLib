#include "ui/framework/CommonControls.h"

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

void init_common_controls() {
    INITCOMMONCONTROLSEX controls{.dwSize = sizeof(INITCOMMONCONTROLSEX), .dwICC = ICC_BAR_CLASSES};
    platform::win32::check(InitCommonControlsEx(&controls), "InitCommonControlsEx");
}

}
