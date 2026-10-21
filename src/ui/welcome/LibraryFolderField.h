#pragma once

#include <filesystem>
#include <functional>

#include <windows.h>

#include "ui/framework/Button.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/Edit.h"
#include "ui/framework/Label.h"
#include "ui/framework/MessageFont.h"
#include "ui/framework/Panel.h"

namespace westie::ui {

class LibraryFolderField final : public framework::Panel {
public:
    LibraryFolderField(HWND parent, std::filesystem::path default_folder, std::function<void(const std::filesystem::path&)> on_change);

    int height_for(int width) const;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    void set_folder(const std::filesystem::path& folder);
    void commit();

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    int row_height() const noexcept;
    int hint_height(int width) const;
    void layout();
    void on_command(WPARAM wparam, LPARAM lparam);
    void show_hint();
    void browse();

    std::filesystem::path default_folder_;
    std::function<void(const std::filesystem::path&)> on_change_;
    std::filesystem::path folder_;
    framework::Edit edit_;
    framework::Button browse_;
    framework::Label hint_;
};

}
