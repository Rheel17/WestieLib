#pragma once

#include <functional>
#include <string>

#include <windows.h>

#include "core/sources/SourceType.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/LinkLabel.h"
#include "ui/framework/MessageFont.h"
#include "ui/framework/Window.h"
#include "ui/inspector/IconText.h"

namespace westie::ui {

class SourceLink final : public framework::Window {
public:
    SourceLink(HWND parent, std::function<void()> on_click);

    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    void set_source(core::SourceType type);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void layout();
    void paint() const;

    IconText name_;
    framework::LinkLabel link_;
};

}
