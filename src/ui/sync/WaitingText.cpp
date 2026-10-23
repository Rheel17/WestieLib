#include "ui/sync/WaitingText.h"

#include "platform/win32/Utf16.h"

namespace westie::ui {

std::wstring waiting_for_text(std::span<const core::ToolPackage> tools) {
    std::wstring text = L"Waiting for ";
    for (std::size_t index = 0; index < tools.size(); ++index) {
        if (index > 0) {
            text += index + 1 == tools.size() ? L" and " : L", ";
        }
        text += platform::win32::to_utf16(core::tool_package_name(tools[index]));
    }
    return text;
}

}
