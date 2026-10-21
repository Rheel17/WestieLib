#pragma once

#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <windows.h>

#include "ui/framework/Control.h"
#include "ui/framework/IconResource.h"
#include "ui/framework/Rect.h"
#include "ui/framework/Size.h"
#include "ui/framework/Tooltip.h"

namespace westie::ui::framework {

enum class TextAlignment { Left, Center };

enum class TextEllipsis { End, Path };

struct PartFormat {
    TextAlignment alignment = TextAlignment::Left;
    TextEllipsis ellipsis = TextEllipsis::End;
    // Drawn after the text; the owner keeps it alive
    const IconResource* icon = nullptr;
};

struct StatusParts {
    std::vector<int> left_widths{};
    std::vector<int> right_widths{};
    // The left part that narrows when the window is too narrow for all of them
    int yielding = 0;
};

class StatusBar final : public Control {
public:
    explicit StatusBar(HWND parent);

    void set_parts(StatusParts parts);
    void set_spacing(int padding, int gap);
    void set_text(int part, std::wstring text, const PartFormat& format = {});
    void set_tip(int part, std::wstring text);
    void embed(int part, HWND window, std::optional<Size> size);
    int text_width(std::wstring_view text) const;
    void draw(const DRAWITEMSTRUCT& item) const;

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    struct PartText {
        std::wstring text{};
        PartFormat format{};
    };

    struct Embedded {
        HWND window = nullptr;
        std::optional<Size> size{};
    };

    void apply_parts();
    void place_embedded() const;
    void place_tips();
    bool is_last_part(int part) const noexcept;
    Rect part_rect(int part) const;
    Rect padded_rect(int part) const;

    StatusParts parts_;
    int padding_ = 0;
    int gap_ = 0;
    std::vector<PartText> texts_;
    std::map<int, std::wstring> tips_;
    std::map<int, Embedded> embedded_;
    Tooltip tooltip_;
};

}
