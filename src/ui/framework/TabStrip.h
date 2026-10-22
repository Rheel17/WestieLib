#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <windows.h>

#include "ui/framework/Control.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/ItemTracker.h"
#include "ui/framework/MessageFont.h"

namespace westie::ui::framework {

class TabStrip final : public Control {
public:
    explicit TabStrip(HWND parent);

    void set_tabs(std::span<const std::wstring> texts, int selected);
    int selected() const;
    int height() const noexcept;
    void apply_dpi(Dpi dpi, const MessageFont& font);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    int count() const;
    RECT item_rect(int index) const;
    std::optional<std::size_t> item_at(LPARAM point) const;
    void paint(HDC dc) const;
    void on_paint() const;
    void on_mouse_move(LPARAM point);
    void on_mouse_leave();

    std::vector<std::wstring> texts_;
    HFONT font_ = nullptr;
    int height_ = 0;
    ItemTracker tracker_;
};

}
