#pragma once

#include <span>
#include <string>
#include <vector>

#include <windows.h>

#include "ui/framework/Control.h"
#include "ui/framework/Rect.h"

namespace westie::ui::framework {

enum class TextAlignment { Left, Center };

enum class TextEllipsis { End, Path };

struct PartFormat {
    TextAlignment alignment = TextAlignment::Left;
    TextEllipsis ellipsis = TextEllipsis::End;
};

class StatusBar final : public Control {
public:
    explicit StatusBar(HWND parent);

    void set_parts(std::span<const int> left_widths, int right_width);
    void set_text_padding(int padding);
    void set_text(int part, std::wstring text, const PartFormat& format = {});
    void draw(const DRAWITEMSTRUCT& item) const;

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    struct PartText {
        std::wstring text{};
        PartFormat format{};
    };

    void apply_parts();
    bool is_right_part(int part) const noexcept;
    Rect part_rect(int part) const;

    std::vector<int> left_widths_;
    int right_width_ = 0;
    int text_padding_ = 0;
    std::vector<PartText> texts_;
};

}
