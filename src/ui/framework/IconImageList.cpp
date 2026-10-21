#include "ui/framework/IconImageList.h"

#include "platform/win32/Win32Error.h"
#include "ui/framework/Instance.h"

namespace westie::ui::framework {

IconImageList::IconImageList(std::span<const int> resource_ids, int size) :
        list_(platform::win32::check(ImageList_Create(size, size, ILC_COLOR32, static_cast<int>(resource_ids.size()), 0), "ImageList_Create")) {
    for (int resource_id : resource_ids) {
        HANDLE loaded = LoadImageW(module_instance(), MAKEINTRESOURCEW(resource_id), IMAGE_ICON, size, size, LR_DEFAULTCOLOR);
        platform::win32::UniqueIcon icon(static_cast<HICON>(platform::win32::check(loaded, "LoadImageW")));
        if (ImageList_ReplaceIcon(list_.get(), -1, icon.get()) == -1) {
            platform::win32::throw_last_error("ImageList_ReplaceIcon");
        }
    }
}

HIMAGELIST IconImageList::handle() const noexcept {
    return list_.get();
}

}
