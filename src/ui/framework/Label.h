#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <windows.h>

#include "ui/framework/TextColor.h"
#include "ui/framework/TextEllipsis.h"
#include "ui/framework/Window.h"

namespace westie::ui::framework {

enum class LabelAlignment { Left, Center };

struct LabelStyle {
    TextColor color = TextColor::Normal;
    LabelAlignment alignment = LabelAlignment::Left;
    bool wrap = false;
    TextEllipsis ellipsis = TextEllipsis::End;
    // The window color unless the label sits on something else
    std::optional<COLORREF> background{};
};

class Label final : public Window {
public:
    Label(HWND parent, std::wstring text, const LabelStyle& style = {});

    void set_font(HFONT font);
    void set_text(std::wstring text);
    void set_color(TextColor color);
    int height_for(int width) const;
    int height_for(std::wstring_view text, int width) const;
    int text_width() const;

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
