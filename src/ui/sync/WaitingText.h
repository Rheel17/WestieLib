#pragma once

#include <span>
#include <string>

#include "core/tools/ToolPackage.h"

namespace westie::ui {

std::wstring waiting_for_text(std::span<const core::ToolPackage> tools);

}
