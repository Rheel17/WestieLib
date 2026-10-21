#pragma once

#include <functional>
#include <optional>

#include <windows.h>

#include "core/settings/Settings.h"
#include "ui/dialogs/ToolListView.h"
#include "ui/framework/ModalDialog.h"
#include "ui/tools/ToolBoard.h"

namespace westie::ui {

class SettingsDialog final : public framework::ModalDialog {
public:
    SettingsDialog(core::Settings settings, const ToolBoard& tools, std::function<void()> on_check_for_updates);

    std::optional<core::Settings> edit(HWND owner);
    void show_tools(bool can_check_for_updates);

protected:
    INT_PTR handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void on_init_dialog();
    INT_PTR on_static_color(HDC dc, HWND control) const;
    void on_command(WPARAM wparam);
    void change_library_folder();
    void show_library_folder() const;
    void show_tool_list();
    void accept();

    core::Settings settings_;
    const ToolBoard& tools_;
    std::function<void()> on_check_for_updates_;
    bool can_check_for_updates_ = false;
    std::optional<ToolListView> tool_list_;
};

}
