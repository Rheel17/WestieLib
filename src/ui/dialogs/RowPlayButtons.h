#pragma once

#include <cstddef>
#include <functional>
#include <optional>

#include <windows.h>

#include "ui/framework/IconResource.h"
#include "ui/framework/ItemTracker.h"
#include "ui/framework/Tooltip.h"

namespace westie::ui {

class RowPlayButtons {
public:
    // Gets the row whose button was clicked
    RowPlayButtons(HWND list, int column, std::function<void(std::size_t)> on_play);

    void apply_dpi();
    // A mouse message the list mustn't see gets its result
    std::optional<LRESULT> on_mouse(UINT message, LPARAM lparam);
    // After the list scrolled under a mouse that stood still
    void follow_cursor();
    void paint(HDC dc, int item) const;

private:
    RECT cell(int item) const;
    RECT box(int item) const;
    std::optional<std::size_t> item_at(POINT point) const;
    std::optional<std::size_t> item_at(LPARAM point) const;
    void repaint(bool changed, std::optional<std::size_t> hot_before);

    HWND list_ = nullptr;
    int column_ = 0;
    std::function<void(std::size_t)> on_play_;
    framework::Tooltip tooltip_;
    framework::ItemTracker mouse_;
    std::optional<framework::IconResource> icon_;
};

}
