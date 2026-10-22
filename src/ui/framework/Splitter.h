#pragma once

#include <functional>
#include <optional>

#include <windows.h>

#include "ui/framework/Window.h"

namespace westie::ui::framework {

enum class SplitterAxis { X, Y };

class Splitter final : public Window {
public:
    Splitter(HWND parent, SplitterAxis axis, std::function<void(int position)> on_drag);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const WindowClass& window_class();

    int along_axis(LPARAM lparam) const noexcept;
    bool on_set_cursor(LPARAM lparam) const;
    void on_button_down(LPARAM lparam);
    void on_mouse_move(WPARAM keys, LPARAM lparam);
    void on_button_up();

    SplitterAxis axis_;
    std::function<void(int position)> on_drag_;
    std::optional<int> grab_offset_;
};

}
