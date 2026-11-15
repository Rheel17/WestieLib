#pragma once

#include <optional>

#include <windows.h>

#include "ui/framework/IconResource.h"
#include "ui/framework/Tooltip.h"

namespace westie::ui {

class PlayButton {
public:
    PlayButton(HWND button, const wchar_t* tip);

    void set_enabled(bool enabled);
    void apply_dpi();

private:
    void show_icon(const framework::IconResource& icon, const framework::IconResource& disabled_icon) const;

    HWND button_ = nullptr;
    framework::Tooltip tooltip_;
    std::optional<framework::IconResource> icon_;
    std::optional<framework::IconResource> disabled_icon_;
};

}
