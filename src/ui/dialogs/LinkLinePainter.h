#pragma once

#include <windows.h>

#include "ui/dialogs/LinkLine.h"

namespace westie::ui {

void paint_link_line(const DRAWITEMSTRUCT& item, const LinkLine& line, double spinner_turn);

}
