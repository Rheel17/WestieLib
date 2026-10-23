#pragma once

#include <set>
#include <string>

#include <windows.h>

#include "ui/framework/Control.h"
#include "ui/framework/Rect.h"

namespace westie::ui::framework {

class Tooltip final : public Control {
public:
    explicit Tooltip(HWND owner);

    void set_tool(UINT_PTR id, const Rect& bounds, const std::wstring& text);
    void remove_tool(UINT_PTR id);

private:
    HWND owner_ = nullptr;
    std::set<UINT_PTR> tools_;
};

}
