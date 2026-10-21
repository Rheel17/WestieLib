#include "ui/framework/ProgressBar.h"

#include <windows.h>
#include <commctrl.h>

namespace westie::ui::framework {

ProgressBar::ProgressBar(HWND parent) : Control({.class_name = PROGRESS_CLASSW, .style = WS_CHILD, .parent = parent}) {}

void ProgressBar::set_percent(int percent) {
    SendMessageW(hwnd(), PBM_SETPOS, percent, 0);
}

}
