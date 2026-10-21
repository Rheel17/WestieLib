#include "ui/tools/ToolMark.h"

namespace westie::ui {

ToolMark tool_mark(const core::ToolStatus& status) noexcept {
    switch (status.state) {
    case core::ToolState::Installed:
    case core::ToolState::UpToDate:    return ToolMark::Ready;
    case core::ToolState::Checking:
    case core::ToolState::Downloading:
    case core::ToolState::Installing:  return ToolMark::Working;
    case core::ToolState::Missing:
    case core::ToolState::Failed:      return ToolMark::Attention;
    }
    return ToolMark::Attention;
}

}
