#pragma once

#include <string>
#include <string_view>

#include <windows.h>

#include "ui/framework/Window.h"

namespace westie::ui::framework {

enum class TextColor { Normal, Gray, Error };

enum class LabelAlignment { Left, Center };

struct LabelStyle {
    TextColor color = TextColor::Normal;
    LabelAlignment alignment = LabelAlignment::Left;
    bool wrap = false;
};

class Label final : public Window {
public:
    Label(HWND parent, std::wstring text, const LabelStyle& style = {});

    void set_font(HFONT font);
    void set_text(std::wstring text);
    void set_color(TextColor color);
    int height_for(int width) const;
    int height_for(std::wstring_view text, int width) const;

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const WindowClass& window_class();

    UINT format() const noexcept;
    void paint() const;

    std::wstring text_;
    LabelStyle style_;
    HFONT font_ = nullptr;
};

}
