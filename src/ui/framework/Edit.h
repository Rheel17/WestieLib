#pragma once

#include <string>

#include <windows.h>

#include "ui/framework/Control.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/MessageFont.h"

namespace westie::ui::framework {

class Edit final : public Control {
public:
    explicit Edit(HWND parent);

    int height() const noexcept;
    void apply_dpi(Dpi dpi, const MessageFont& font);
    std::wstring text() const;
    void set_text(const std::wstring& text);

private:
    int height_ = 0;
};

}
