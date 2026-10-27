#include "ui/main/MainAccelerators.h"

#include <array>
#include <utility>

#include <windows.h>

#include "ui/commands/CommandId.h"

namespace westie::ui {

namespace {

constexpr ACCEL shortcut(BYTE modifiers, WORD key, CommandId command) {
    return {.fVirt = static_cast<BYTE>(FVIRTKEY | modifiers), .key = key, .cmd = std::to_underlying(command)};
}

// Enter belongs to the track list, which handles it without taking it from other controls
constexpr std::array Shortcuts = {
    shortcut(FCONTROL, 'N', CommandId::AddPlaylist),
    shortcut(FCONTROL, 'Q', CommandId::Exit),
    shortcut(0, VK_F5, CommandId::SyncAll),
    shortcut(FCONTROL, 'L', CommandId::ToggleLog),
    shortcut(FCONTROL, 'F', CommandId::FocusSearch),
    shortcut(FCONTROL | FALT, 'S', CommandId::Settings),
};

}

framework::AcceleratorTable build_main_accelerators() {
    return framework::AcceleratorTable(Shortcuts);
}

}
