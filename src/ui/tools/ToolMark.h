#pragma once

#include "core/tools/ToolStatus.h"

namespace westie::ui {

enum class ToolMark { Ready, Working, Attention };

ToolMark tool_mark(const core::ToolStatus& status) noexcept;

}
