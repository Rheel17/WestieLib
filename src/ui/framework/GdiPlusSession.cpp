#include "ui/framework/GdiPlusSession.h"

#include <format>
#include <stdexcept>

#include <objidl.h>
#include <gdiplus.h>

namespace westie::ui::framework {

GdiPlusSession::GdiPlusSession() {
    Gdiplus::GdiplusStartupInput input;
    if (Gdiplus::Status status = Gdiplus::GdiplusStartup(&token_, &input, nullptr); status != Gdiplus::Ok) {
        throw std::runtime_error(std::format("GdiplusStartup failed with status {}", static_cast<int>(status)));
    }
}

GdiPlusSession::~GdiPlusSession() {
    Gdiplus::GdiplusShutdown(token_);
}

}
