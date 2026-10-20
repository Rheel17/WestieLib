#pragma once

#include <string>

#include <windows.h>

namespace westie::ui::framework {

class WindowClass;

class Window {
public:
    virtual ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    HWND hwnd() const noexcept;
    virtual void show(int show_command);

protected:
    struct CreateParams {
        std::wstring title{};
        DWORD style = 0;
        DWORD ex_style = 0;
        HWND parent = nullptr;
    };

    Window(const WindowClass& window_class, const CreateParams& params);

    virtual LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam);

private:
    friend class WindowClass;

    static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    void destroy() noexcept;

    HWND hwnd_ = nullptr;
};

}
