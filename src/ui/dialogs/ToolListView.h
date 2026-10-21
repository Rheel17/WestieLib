#pragma once

#include <string>

#include <windows.h>

#include "ui/framework/IconImageList.h"
#include "ui/tools/ToolBoard.h"

namespace westie::ui {

class ToolListView {
public:
    explicit ToolListView(HWND list);

    void show(const ToolBoard& board);

private:
    void add_columns();
    void set_text(int item, int column, const std::wstring& text);

    HWND list_ = nullptr;
    framework::IconImageList icons_;
};

}
