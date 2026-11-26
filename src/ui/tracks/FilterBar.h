#pragma once

#include <functional>
#include <optional>
#include <string>

#include <windows.h>

#include "core/sources/SourceType.h"
#include "ui/framework/CloseButton.h"
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
    FilterBar(HWND parent, std::function<void()> on_change);

    int height() const;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    std::wstring search() const;
    std::optional<core::SourceType> source() const;
    void clear();
    void focus_search();

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    int row_height() const;
    void layout();
    void place_search_icons();
    void show_search_clear();
    void erase_search();
    void on_command(WPARAM wparam, LPARAM lparam);

    std::function<void()> on_change_;
    framework::Edit search_;
    framework::IconView search_icon_;
    framework::CloseButton search_clear_;
    framework::Label source_label_;
    framework::ComboBox source_;
    int label_height_ = 0;
};

}
