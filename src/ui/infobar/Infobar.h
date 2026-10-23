#pragma once

#include <functional>
#include <optional>
#include <string>

#include <windows.h>

#include "ui/framework/Button.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/Label.h"
#include "ui/framework/MessageFont.h"
#include "ui/framework/Rect.h"
#include "ui/framework/Window.h"

namespace westie::ui {

class Infobar final : public framework::Window {
public:
    Infobar(HWND parent, std::function<void()> on_details, std::function<void()> on_dismiss);

    int height() const noexcept;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    void set_message(std::wstring message);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    framework::Rect strip() const;
    framework::Rect close_box() const;
    void layout();
    void paint() const;
    void on_left_button_down(POINT point);
    void on_left_button_up(POINT point);
    void on_command(WPARAM wparam) const;

    std::function<void()> on_details_;
    std::function<void()> on_dismiss_;
    framework::Label message_;
    framework::Button details_;
    std::optional<framework::IconResource> warn_;
    std::optional<framework::IconResource> close_;
    bool close_pressed_ = false;
    int line_height_ = 0;
    int height_ = 0;
};

}
