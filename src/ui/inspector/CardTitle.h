#pragma once

#include <optional>
#include <string>

#include <windows.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/FontSet.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/Window.h"

namespace westie::ui {

class CardTitle final : public framework::Window {
public:
    explicit CardTitle(HWND parent);

    int height() const noexcept;
    int text_left() const;
    void apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts);
    void set(int icon, std::wstring text);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void load_icon(framework::Dpi dpi);
    void paint() const;

    int icon_id_ = 0;
    std::optional<framework::IconResource> icon_;
    std::wstring text_;
    HFONT font_ = nullptr;
    int height_ = 0;
};

}
