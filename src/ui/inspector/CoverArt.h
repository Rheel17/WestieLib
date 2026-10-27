#pragma once

#include <optional>

#include <windows.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/Window.h"

namespace westie::ui {

class CoverArt final : public framework::Window {
public:
    explicit CoverArt(HWND parent);

    void apply_dpi(framework::Dpi dpi);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void paint() const;

    std::optional<framework::IconResource> note_;
};

}
