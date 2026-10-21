#pragma once

#include <windows.h>

#include "platform/win32/UniqueHandle.h"
#include "ui/framework/Dpi.h"

namespace westie::ui::framework {

enum class FontStyle { Regular, Semibold };

class MessageFont {
public:
    explicit MessageFont(Dpi dpi, FontStyle style = FontStyle::Regular);

    HFONT handle() const noexcept;
    int line_height() const noexcept;

private:
    platform::win32::UniqueFont font_;
    int line_height_ = 0;
};

}
