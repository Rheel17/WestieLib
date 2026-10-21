#pragma once

#include <optional>

#include <windows.h>

#include "core/Settings.h"
#include "ui/framework/ModalDialog.h"

namespace westie::ui {

class SettingsDialog final : public framework::ModalDialog {
public:
    explicit SettingsDialog(core::Settings settings);

    std::optional<core::Settings> edit(HWND owner);

protected:
    INT_PTR handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void on_init_dialog();
    INT_PTR on_static_color(HDC dc, HWND control) const;
    void on_command(WPARAM wparam);

    core::Settings settings_;
};

}
