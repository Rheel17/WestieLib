#pragma once

#include <chrono>
#include <string>

#include <windows.h>

#include "ui/framework/Window.h"

namespace westie::ui {

class LengthValue final : public framework::Window {
public:
    explicit LengthValue(HWND parent);

    void set_font(HFONT font);
    void set_lengths(std::chrono::milliseconds video, std::chrono::milliseconds source);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void paint() const;

    std::wstring video_;
    std::wstring source_;
    // A length that far off often gives away a live version or a cover
    bool off_ = false;
    HFONT font_ = nullptr;
};

}
