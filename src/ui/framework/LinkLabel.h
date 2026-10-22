#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>

#include <windows.h>

#include "platform/win32/UniqueHandle.h"
#include "ui/framework/ItemTracker.h"
#include "ui/framework/Window.h"

namespace westie::ui::framework {

class LinkLabel final : public Window {
public:
    LinkLabel(HWND parent, std::function<void()> on_click);

    void set_font(HFONT font);
    void set_text(std::wstring text);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const WindowClass& window_class();

    int text_width() const;
    std::optional<std::size_t> item_at(POINT point) const;
    void repaint_if(bool changed) const;
    bool on_set_cursor() const;
    void on_left_button_up(LPARAM lparam);
    void paint() const;

    std::function<void()> on_click_;
    std::wstring text_;
    HFONT font_ = nullptr;
    platform::win32::UniqueFont underlined_;
    ItemTracker mouse_;
};

}
