#pragma once

#include <span>

#include <windows.h>
#include <commctrl.h>

#include "platform/win32/UniqueHandle.h"

namespace westie::ui::framework {

class IconImageList {
public:
    IconImageList() = default;
    IconImageList(std::span<const int> resource_ids, int size);

    HIMAGELIST handle() const noexcept;

private:
    platform::win32::UniqueImageList list_;
};

}
