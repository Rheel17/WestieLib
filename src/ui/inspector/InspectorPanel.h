#pragma once

#include <windows.h>

#include "ui/framework/FontSet.h"
#include "ui/framework/Label.h"
#include "ui/framework/Panel.h"

namespace westie::ui {

class InspectorPanel final : public framework::Panel {
public:
    explicit InspectorPanel(HWND parent);

    void apply_fonts(const framework::FontSet& fonts);
    void show_placeholder(bool shown);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void layout();

    framework::Label placeholder_;
};

}
