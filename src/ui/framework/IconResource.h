#pragma once

#include <windows.h>

#include "platform/win32/UniqueHandle.h"

namespace westie::ui::framework {

class IconResource {
public:
    IconResource(int resource_id, int size);

    int size() const noexcept;
    void draw(HDC dc, int x, int y) const;

private:
    platform::win32::UniqueIcon icon_;
    int size_ = 0;
};

}
