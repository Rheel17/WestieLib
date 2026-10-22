#pragma once

#include <windows.h>

#include "ui/framework/Rect.h"

namespace westie::ui {

enum class RowLook { Normal, Hot, Pressed, Selected };

RowLook row_look(bool selected, bool hot, bool pressed) noexcept;
void paint_row_look(HDC dc, const framework::Rect& bounds, RowLook look);

}
