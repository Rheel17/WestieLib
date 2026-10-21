#pragma once

#include <memory>
#include <type_traits>

#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <uxtheme.h>

namespace westie::platform::win32 {

template<auto Close>
struct HandleCloser {
    void operator()(auto handle) const noexcept { Close(handle); }
};

template<typename Handle, auto Close>
using UniqueHandle = std::unique_ptr<std::remove_pointer_t<Handle>, HandleCloser<Close>>;

using UniqueIcon = UniqueHandle<HICON, DestroyIcon>;
using UniqueMenu = UniqueHandle<HMENU, DestroyMenu>;
using UniqueKey = UniqueHandle<HKEY, RegCloseKey>;
using UniqueAcceleratorTable = UniqueHandle<HACCEL, DestroyAcceleratorTable>;
using UniqueFont = UniqueHandle<HFONT, DeleteObject>;
using UniqueBitmap = UniqueHandle<HBITMAP, DeleteObject>;
using UniqueMemoryDc = UniqueHandle<HDC, DeleteDC>;
using UniqueImageList = UniqueHandle<HIMAGELIST, ImageList_Destroy>;
using UniqueCoTaskString = UniqueHandle<PWSTR, CoTaskMemFree>;
using UniqueTheme = UniqueHandle<HTHEME, CloseThemeData>;

}
