#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <windows.h>

#include "platform/win32/UniqueHandle.h"
#include "ui/framework/ItemTracker.h"
#include "ui/framework/Window.h"

namespace westie::ui::framework {

class LinkLabel final : public Window {
public:
    // Gets the index of the link clicked
    LinkLabel(HWND parent, std::function<void(std::size_t)> on_click);

    void set_font(HFONT font);
    void set_text(std::wstring text);
    void set_links(std::vector<std::wstring> links);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    struct Span {
        int left = 0;
        int right = 0;
    };

    static const WindowClass& window_class();

    // Where each link that starts inside the window runs, cut off at its right edge
    std::vector<Span> spans() const;
    std::optional<std::size_t> item_at(POINT point) const;
    void repaint_if(bool changed) const;
    bool on_set_cursor() const;
    void on_left_button_up(LPARAM lparam);
    void paint() const;

    std::function<void(std::size_t)> on_click_;
    std::vector<std::wstring> links_;
    HFONT font_ = nullptr;
    platform::win32::UniqueFont underlined_;
    ItemTracker mouse_;
};

}
