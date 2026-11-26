#pragma once

#include <optional>

#include <windows.h>

namespace westie::ui {

enum class LogLineAction : WORD { GoToTrack = 1, GoToPlaylist, Copy };

struct LogLineMenu {
    // What the line is about
    bool track = false;
    bool playlist = false;
};

std::optional<LogLineAction> choose_log_line_action(HWND owner, POINT screen, const LogLineMenu& menu);

}
