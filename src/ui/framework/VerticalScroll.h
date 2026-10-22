#pragma once

#include <windows.h>

namespace westie::ui::framework {

class VerticalScroll {
public:
    int position() const noexcept;
    void update(HWND window, int content_height);
    bool scroll_to(HWND window, int position);
    bool reveal(HWND window, int top, int bottom);
    bool on_scroll(HWND window, WORD request);
    bool on_wheel(HWND window, short delta);

private:
    int content_height_ = 0;
    int position_ = 0;
    int wheel_remainder_ = 0;
};

}
