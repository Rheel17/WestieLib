#pragma once

#include <windows.h>

namespace westie::ui::framework {

enum class TextColor { Normal, Gray, Error, Warning };

COLORREF text_color(TextColor color);

}
