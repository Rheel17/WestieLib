#pragma once

#include <windows.h>

namespace westie::ui::framework {

class Control {
public:
    virtual ~Control();

    Control(const Control&) = delete;
    Control& operator=(const Control&) = delete;

    HWND hwnd() const noexcept;
    void set_font(HFONT font);

protected:
    struct CreateParams {
        const wchar_t* class_name = nullptr;
        DWORD style = 0;
        DWORD ex_style = 0;
        HWND parent = nullptr;
    };

    explicit Control(const CreateParams& params);

    virtual LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam);

private:
    static LRESULT CALLBACK subclass_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data);

    HWND hwnd_ = nullptr;
};

}
