#pragma once

#include <string>

#include <windows.h>

#include "ui/framework/Window.h"

namespace westie::ui {

class SectionTitle final : public framework::Window {
public:
    SectionTitle(HWND parent, std::wstring text);

    void set_font(HFONT font);
    void set_text(std::wstring text);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void paint() const;

    std::wstring text_;
    HFONT font_ = nullptr;
};

}
