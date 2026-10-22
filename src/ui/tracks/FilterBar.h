#pragma once

#include <functional>

#include <windows.h>

#include "ui/framework/Button.h"
#include "ui/framework/ComboBox.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/Edit.h"
#include "ui/framework/IconView.h"
#include "ui/framework/Label.h"
#include "ui/framework/MessageFont.h"
#include "ui/framework/Panel.h"

namespace westie::ui {

class FilterBar final : public framework::Panel {
public:
    FilterBar(HWND parent, std::function<void()> on_change, std::function<void()> on_clear);

    int height() const;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    bool is_default() const;
    void clear();
    void set_clear_enabled(bool enabled);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    int row_height() const;
    void layout();
    void place_search_icon();
    void on_command(WPARAM wparam, LPARAM lparam);

    std::function<void()> on_change_;
    std::function<void()> on_clear_;
    framework::Edit search_;
    framework::IconView search_icon_;
    framework::Label source_label_;
    framework::ComboBox source_;
    framework::Button clear_;
    int label_height_ = 0;
};

}
