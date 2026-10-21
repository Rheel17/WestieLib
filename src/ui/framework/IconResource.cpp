#include "ui/framework/IconResource.h"

#include "platform/win32/Win32Error.h"
#include "ui/framework/Instance.h"

namespace westie::ui::framework {

IconResource::IconResource(int resource_id, int size) :
        icon_(static_cast<HICON>(
                platform::win32::check(LoadImageW(module_instance(), MAKEINTRESOURCEW(resource_id), IMAGE_ICON, size, size, LR_DEFAULTCOLOR), "LoadImageW")
        )),
        size_(size) {}

int IconResource::size() const noexcept {
    return size_;
}

void IconResource::draw(HDC dc, int x, int y) const {
    platform::win32::check(DrawIconEx(dc, x, y, icon_.get(), size_, size_, 0, nullptr, DI_NORMAL), "DrawIconEx");
}

}
