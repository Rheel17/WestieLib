#pragma once

#include <windows.h>

namespace westie::ui::framework {

class GdiPlusSession {
public:
    GdiPlusSession();
    ~GdiPlusSession();

    GdiPlusSession(const GdiPlusSession&) = delete;
    GdiPlusSession& operator=(const GdiPlusSession&) = delete;

private:
    ULONG_PTR token_ = 0;
};

}
