#pragma once

#include <functional>

#include <windows.h>
#include <commctrl.h>

#include "ui/commands/CommandId.h"
#include "ui/commands/CommandState.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/IconImageList.h"
#include "ui/framework/MessageFont.h"
#include "ui/framework/Toolbar.h"
#include "ui/framework/Window.h"

namespace westie::ui {

class MainToolbar final : public framework::Window {
public:
    MainToolbar(HWND parent, std::function<void(CommandId)> on_command);

    int height() const noexcept;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    void update(const CommandState& state);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void layout();
    void paint();
    LRESULT on_notify(const NMHDR& header);
    LRESULT on_custom_draw(const NMCUSTOMDRAW& draw) const;
    void paint_separators(HDC dc) const;
    void show_sync_menu(const NMTOOLBARW& notification);

    std::function<void(CommandId)> on_command_;
    framework::Toolbar toolbar_;
    framework::IconImageList images_;
    framework::IconImageList disabled_images_;
    CommandState state_;
    int height_ = 0;
};

}
