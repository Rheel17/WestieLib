#pragma once

#include <string>

#include <windows.h>

#include "ui/framework/Control.h"
#include "ui/framework/Dpi.h"
#include "ui/framework/MessageFont.h"

namespace westie::ui::framework {

struct EditOptions {
    const wchar_t* cue_banner = nullptr;
    // Room before the text, for an icon inside the field
    int leading_dips = 0;
};

class Edit final : public Control {
public:
    explicit Edit(HWND parent, const EditOptions& options = {});

    int height() const noexcept;
    void apply_dpi(Dpi dpi, const MessageFont& font);
    std::wstring text() const;
    void set_text(const std::wstring& text);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    int leading_dips_ = 0;
    int height_ = 0;
    int text_height_ = 0;
};

}
