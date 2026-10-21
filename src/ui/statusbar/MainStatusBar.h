#pragma once

#include <filesystem>

#include <windows.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/MessageFont.h"
#include "ui/framework/StatusBar.h"

namespace westie::ui {

class MainStatusBar {
public:
    explicit MainStatusBar(HWND parent);

    HWND hwnd() const noexcept;
    int height() const noexcept;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    void set_track_count(int count);
    void set_library_folder(const std::filesystem::path& folder);
    void draw(const DRAWITEMSTRUCT& item) const;

private:
    void set_parts(framework::Dpi dpi);

    framework::StatusBar status_bar_;
    int height_ = 0;
};

}
