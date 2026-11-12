#pragma once

#include <functional>

#include <windows.h>
#include <commctrl.h>

#include "core/log/LogLevel.h"
#include "ui/framework/Button.h"
#include "ui/framework/CloseButton.h"
#include "ui/framework/ComboBox.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/Label.h"
#include "ui/framework/Panel.h"
#include "ui/framework/TabStrip.h"
#include "ui/log/LogFilter.h"

namespace westie::ui {

class LogToolbar final : public framework::Panel {
public:
    struct Callbacks {
        std::function<void(LogView)> on_view_change;
        std::function<void()> on_open_folder;
        std::function<void()> on_close;
    };

    LogToolbar(HWND parent, Callbacks callbacks);

    int height() const;
    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    LogView view() const;
    void set_view(LogView view);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    int row_height() const;
    void layout();
    void paint() const;
    void on_command(WPARAM wparam, LPARAM lparam);
    LRESULT on_notify(const NMHDR& header);

    Callbacks callbacks_;
    framework::Label title_;
    framework::Label level_label_;
    framework::ComboBox level_;
    framework::TabStrip scope_;
    framework::Button open_folder_;
    framework::CloseButton close_;
    int line_height_ = 0;
    int title_line_height_ = 0;
};

}
