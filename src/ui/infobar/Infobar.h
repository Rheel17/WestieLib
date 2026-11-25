#pragma once

#include <functional>
#include <optional>
#include <string>

#include <windows.h>

#include "ui/framework/Button.h"
#include "ui/framework/CloseButton.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/Label.h"
#include "ui/framework/MessageFont.h"
#include "ui/framework/Rect.h"
#include "ui/framework/Window.h"

namespace westie::ui {

enum class InfobarAction { Details, Review };

class Infobar final : public framework::Window {
public:
    Infobar(HWND parent, std::function<void()> on_action, std::function<void()> on_dismiss);

    int height() const noexcept;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    void set_message(std::wstring message, InfobarAction action);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    framework::Rect strip() const;
    void layout();
    void paint() const;
    void on_command(WPARAM wparam) const;

    std::function<void()> on_action_;
    std::function<void()> on_dismiss_;
    framework::Label message_;
    framework::Button details_;
    framework::Button review_;
    framework::CloseButton close_;
    std::optional<framework::IconResource> warn_;
    InfobarAction action_ = InfobarAction::Details;
    int line_height_ = 0;
    int height_ = 0;
};

}
