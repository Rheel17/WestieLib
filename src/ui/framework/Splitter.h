#pragma once

#include <functional>
#include <optional>

#include <windows.h>

#include "ui/framework/Window.h"

namespace westie::ui::framework {

class Splitter final : public Window {
public:
    Splitter(HWND parent, std::function<void(int left)> on_drag);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const WindowClass& window_class();

    void on_button_down(LPARAM lparam);
    void on_mouse_move(LPARAM lparam);
    void on_button_up();

    std::function<void(int left)> on_drag_;
    std::optional<int> grab_offset_;
};

}
