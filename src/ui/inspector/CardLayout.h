#pragma once

#include <initializer_list>

#include <windows.h>

#include "ui/framework/Button.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/Rect.h"

namespace westie::ui {

class CardLayout {
public:
    CardLayout(framework::Dpi dpi, int width, int line_height, framework::DeferredPositions* positions);

    int height() const noexcept;
    void title(HWND title, int height);
    void subtitle(HWND subtitle, int indent);
    void section(HWND title);
    void row(HWND key, HWND value);
    void buttons(std::initializer_list<const framework::Button*> buttons);

private:
    framework::Rect line(int gap, int indent, int height);
    void place(HWND window, const framework::Rect& rect);

    framework::Dpi dpi_;
    int width_ = 0;
    int line_height_ = 0;
    // Null while the layout only measures, so a card can work out its height for a width
    framework::DeferredPositions* positions_ = nullptr;
    int bottom_ = 0;
    bool first_row_ = true;
};

}
