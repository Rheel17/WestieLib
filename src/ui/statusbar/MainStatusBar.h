#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <windows.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/MessageFont.h"
#include "ui/framework/StatusBar.h"
#include "ui/tools/ToolBoard.h"
#include "ui/tools/ToolMark.h"
#include "ui/tools/ToolStatusText.h"

namespace westie::ui {

class MainStatusBar {
public:
    explicit MainStatusBar(HWND parent);

    HWND hwnd() const noexcept;
    int height() const noexcept;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    void set_track_count(int count);
    void set_library_folder(const std::filesystem::path& folder);
    void set_tools(const ToolBoard& board);
    void draw(const DRAWITEMSTRUCT& item) const;

private:
    void load_icons();
    void measure_tools();
    void show_tools();
    void set_parts();
    const framework::IconResource* tool_icon() const noexcept;
    const framework::IconResource& ring(int percent) const noexcept;

    framework::StatusBar status_bar_;
    framework::Dpi dpi_;
    framework::IconResource ready_icon_;
    framework::IconResource attention_icon_;
    std::vector<framework::IconResource> rings_;
    ToolMark tool_mark_ = ToolMark::Working;
    std::wstring tools_tip_;
    std::optional<ToolWork> work_;
    int tools_width_ = 0;
    int height_ = 0;
};

}
