#pragma once

#include <filesystem>
#include <functional>

#include <windows.h>

#include "ui/commands/CommandId.h"
#include "ui/framework/Button.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/Label.h"
#include "ui/framework/Panel.h"
#include "ui/framework/Rect.h"
#include "ui/welcome/LibraryFolderField.h"

namespace westie::ui {

class WelcomePanel final : public framework::Panel {
public:
    WelcomePanel(
            HWND parent,
            const std::filesystem::path& default_library_folder,
            std::function<void(CommandId)> on_command,
            std::function<void(const std::filesystem::path&)> on_library_folder
    );

    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void set_library_folder(const std::filesystem::path& folder);
    void commit_library_folder();

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void layout();
    void paint() const;
    void on_command(WPARAM wparam) const;

    std::function<void(CommandId)> on_command_;
    framework::IconResource icon_;
    framework::Label heading_;
    framework::Label introduction_;
    framework::Label folder_label_;
    LibraryFolderField folder_;
    framework::Button add_button_;
    framework::Rect icon_rect_;
    int heading_height_ = 0;
    int label_height_ = 0;
};

}
