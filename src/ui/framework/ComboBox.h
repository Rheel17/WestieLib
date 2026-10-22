#pragma once

#include <span>

#include <windows.h>

#include "ui/framework/Control.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/MessageFont.h"

namespace westie::ui::framework {

class ComboBox final : public Control {
public:
    explicit ComboBox(HWND parent);

    void add_items(std::span<const wchar_t* const> items);
    int selected() const;
    void select(int index);
    int height() const noexcept;
    void apply_dpi(Dpi dpi, const MessageFont& font);

private:
    int height_ = 0;
};

}
