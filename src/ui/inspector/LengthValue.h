#pragma once

#include <chrono>
#include <string>
#include <string_view>

#include <windows.h>

#include "ui/framework/Window.h"

namespace westie::ui {

class LengthValue final : public framework::Window {
public:
    explicit LengthValue(HWND parent);

    void set_font(HFONT font);
    void set_lengths(std::chrono::milliseconds result, std::chrono::milliseconds source, std::wstring_view source_name);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void paint() const;

    std::wstring result_;
    std::wstring source_;
    HFONT font_ = nullptr;
};

}
