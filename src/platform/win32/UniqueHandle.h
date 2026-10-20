#pragma once

#include <memory>
#include <type_traits>

#include <windows.h>

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

}
