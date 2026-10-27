#include "ui/framework/TextColor.h"

namespace westie::ui::framework {

namespace {

constexpr COLORREF ErrorColor = RGB(0xC4, 0x2B, 0x1C);
constexpr COLORREF WarningColor = RGB(0x9D, 0x5D, 0x00);

}

COLORREF text_color(TextColor color) {
    switch (color) {
    case TextColor::Gray:    return GetSysColor(COLOR_GRAYTEXT);
    case TextColor::Error:   return ErrorColor;
    case TextColor::Warning: return WarningColor;
    default:                 return GetSysColor(COLOR_WINDOWTEXT);
    }
}

}
