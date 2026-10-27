#pragma once

#include <string>

#include <windows.h>

#include "ui/framework/Window.h"

namespace westie::ui {

class OutputBox final : public framework::Window {
public:
    explicit OutputBox(HWND parent);

    void set_font(HFONT font);
    void set_text(std::wstring text);
    int height_for(int width) const;

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    RECT text_area(RECT box) const;
    void paint() const;

    std::wstring text_;
    HFONT font_ = nullptr;
};

}
