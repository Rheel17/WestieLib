#pragma once

#include <string>

#include <windows.h>

#include "ui/framework/Control.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/IconImageList.h"
#include "ui/framework/MessageFont.h"
#include "ui/framework/Size.h"

namespace westie::ui::framework {

class Button final : public Control {
public:
    struct Options {
        const wchar_t* text = nullptr;
        WORD command = 0;
        int icon = 0;
        bool default_button = false;
        // No minimum width, and less padding
        bool compact = false;
    };

    Button(HWND parent, const Options& options);

    void apply_dpi(Dpi dpi, const MessageFont& font);
    Size size() const noexcept;
    void set_enabled(bool enabled);

private:
    void set_icon(Dpi dpi, int spare_width);
    int content_left_margin() const;
    int text_width(HFONT font) const;

    std::wstring text_;
    int icon_resource_ = 0;
    bool compact_ = false;
    IconImageList icon_;
    Size size_;
};

}
