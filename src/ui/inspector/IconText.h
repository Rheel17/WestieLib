#pragma once

#include <optional>
#include <string>

#include <windows.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/MessageFont.h"
#include "ui/framework/Window.h"

namespace westie::ui {

struct IconTextStyle {
    int gap_dips = 0;
    int line_dips = 0;
};

constexpr IconTextStyle CardTitleStyle{.gap_dips = 7, .line_dips = 20};

class IconText final : public framework::Window {
public:
    IconText(HWND parent, const IconTextStyle& style);

    int height() const noexcept;
    int text_left() const;
    // From the left edge to the end of the text
    int width() const;
    void apply_dpi(framework::Dpi dpi, const framework::MessageFont& font);
    // Icon 0 is none, and the text starts at the left edge
    void set(int icon, std::wstring text);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void load_icon(framework::Dpi dpi);
    void paint() const;

    IconTextStyle style_;
    int icon_id_ = 0;
    std::optional<framework::IconResource> icon_;
    std::wstring text_;
    HFONT font_ = nullptr;
    int height_ = 0;
};

}
