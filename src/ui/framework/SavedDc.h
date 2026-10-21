#pragma once

#include <windows.h>

namespace westie::ui::framework {

class SavedDc {
public:
    explicit SavedDc(HDC dc) noexcept : dc_(dc), state_(SaveDC(dc)) {}
    ~SavedDc() { RestoreDC(dc_, state_); }

    SavedDc(const SavedDc&) = delete;
    SavedDc& operator=(const SavedDc&) = delete;

private:
    HDC dc_;
    int state_;
};

}
