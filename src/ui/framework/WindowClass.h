#pragma once

#include <string>

#include <windows.h>

namespace westie::ui::framework {

class WindowClass {
public:
    struct Options {
        std::wstring name;
        UINT style = 0;
        HBRUSH background = nullptr;
        HICON icon = nullptr;
        HCURSOR cursor = LoadCursorW(nullptr, IDC_ARROW);
    };

    explicit WindowClass(Options options);
    ~WindowClass();

    WindowClass(const WindowClass&) = delete;
    WindowClass& operator=(const WindowClass&) = delete;

    const std::wstring& name() const noexcept;

private:
    std::wstring name_;
};

}
