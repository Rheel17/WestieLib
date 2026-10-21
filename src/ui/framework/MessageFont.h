#pragma once

#include <windows.h>

#include "platform/win32/UniqueHandle.h"
#include "ui/framework/Dpi.h"

namespace westie::ui::framework {

class MessageFont {
public:
    explicit MessageFont(Dpi dpi);

    HFONT handle() const noexcept;
    int line_height() const noexcept;

private:
    platform::win32::UniqueFont font_;
    int line_height_ = 0;
};

}
