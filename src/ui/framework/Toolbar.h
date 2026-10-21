#pragma once

#include <span>
#include <vector>

#include <windows.h>
#include <commctrl.h>

#include "ui/framework/Control.h"
#include "ui/framework/Rect.h"
#include "ui/framework/Size.h"

namespace westie::ui::framework {

struct ToolbarButton {
    WORD command = 0;
    int image = 0;
    const wchar_t* text = nullptr;
    bool dropdown = false;
};

constexpr ToolbarButton ToolbarSeparator{};

struct ToolbarMetrics {
    int button_height = 0;
    int side_padding = 0;
    int icon_gap = 0;
    int button_spacing = 0;
    int separator_width = 0;
};

class Toolbar final : public Control {
public:
    explicit Toolbar(HWND parent);

    void add_buttons(std::span<const ToolbarButton> buttons);
    void set_image_lists(HIMAGELIST normal, HIMAGELIST disabled);
    void apply_metrics(const ToolbarMetrics& metrics, HFONT font);
    void set_enabled(WORD command, bool enabled);
    Size button_size() const;
    std::vector<Rect> separator_rects() const;

private:
    std::vector<int> separator_indexes() const;
    void set_separator_widths(int width);
    void set_padding(int side, int vertical, int button_spacing);
};

}
