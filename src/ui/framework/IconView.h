#pragma once

#include <windows.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/Window.h"

namespace westie::ui::framework {

class IconView final : public Window {
public:
    IconView(HWND parent, int resource_id, int size_dips);

    int size() const noexcept;
    void apply_dpi(Dpi dpi);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const WindowClass& window_class();

    void paint() const;

    int resource_id_ = 0;
    int size_dips_ = 0;
    IconResource icon_;
};

}
