#pragma once

#include <string>

#include <windows.h>

#include "ui/framework/Window.h"

namespace westie::ui {

class ConfidenceValue final : public framework::Window {
public:
    explicit ConfidenceValue(HWND parent);

    void set_font(HFONT font);
    void set_confidence(double confidence, bool review);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void paint() const;

    double confidence_ = 0.0;
    bool review_ = false;
    std::wstring text_;
    HFONT font_ = nullptr;
};

}
