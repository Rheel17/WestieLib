#include "ui/framework/AcceleratorTable.h"

#include <vector>

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

namespace {

HACCEL create_table(std::span<const ACCEL> accelerators) {
    // CreateAcceleratorTableW takes a non-const pointer
    std::vector<ACCEL> entries(accelerators.begin(), accelerators.end());
    return platform::win32::check(CreateAcceleratorTableW(entries.data(), static_cast<int>(entries.size())), "CreateAcceleratorTableW");
}

}

AcceleratorTable::AcceleratorTable(std::span<const ACCEL> accelerators) : table_(create_table(accelerators)) {}

bool AcceleratorTable::translate(HWND window, MSG& message) const {
    return TranslateAcceleratorW(window, table_.get(), &message) != 0;
}

}
