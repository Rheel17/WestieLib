#pragma once

#include <cstddef>
#include <functional>
#include <optional>

#include <windows.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/ItemTracker.h"
#include "ui/framework/Window.h"

namespace westie::ui::framework {

struct CloseButtonOptions {
    int box_dips = 24;
    int icon_dips = 12;
    const wchar_t* title = L"Close";
};

class CloseButton final : public Window {
public:
    CloseButton(HWND parent, COLORREF background, std::function<void()> on_click, const CloseButtonOptions& options = {});

    int size() const noexcept;
    void apply_dpi(Dpi dpi);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const WindowClass& window_class();

    std::optional<std::size_t> item_at(LPARAM point) const;
    void repaint_if(bool changed) const;
    void on_left_button_up(LPARAM point);
    void paint() const;

    COLORREF background_;
    std::function<void()> on_click_;
    int box_dips_ = 0;
    int icon_dips_ = 0;
    std::optional<IconResource> icon_;
    ItemTracker mouse_;
    int size_ = 0;
};

}
