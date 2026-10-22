#pragma once

#include <cstdio>
#include <memory>
#include <type_traits>

#include <windows.h>
#include <bcrypt.h>
#include <commctrl.h>
#include <objbase.h>
#include <uxtheme.h>
#include <winhttp.h>

namespace westie::platform::win32 {

template<auto Close>
struct HandleCloser {
    void operator()(auto handle) const noexcept { Close(handle); }
};

template<typename Handle, auto Close>
using UniqueHandle = std::unique_ptr<std::remove_pointer_t<Handle>, HandleCloser<Close>>;

inline void release_com(IUnknown* object) noexcept {
    object->Release();
}

template<typename Interface>
using UniqueComPtr = UniqueHandle<Interface*, release_com>;

// The standard library's functions can't be template arguments
inline void close_c_file(std::FILE* file) noexcept {
    std::fclose(file);
}

inline void close_algorithm_provider(BCRYPT_ALG_HANDLE algorithm) noexcept {
    BCryptCloseAlgorithmProvider(algorithm, 0);
}

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
using UniqueFile = UniqueHandle<HANDLE, CloseHandle>;
using UniqueKernelObject = UniqueHandle<HANDLE, CloseHandle>;
using UniqueEnvironmentStrings = UniqueHandle<LPWCH, FreeEnvironmentStringsW>;
using UniqueLocalString = UniqueHandle<PWSTR, LocalFree>;
using UniqueCFile = UniqueHandle<std::FILE*, close_c_file>;
using UniqueInternet = UniqueHandle<HINTERNET, WinHttpCloseHandle>;
using UniqueAlgorithmProvider = UniqueHandle<BCRYPT_ALG_HANDLE, close_algorithm_provider>;
using UniqueHash = UniqueHandle<BCRYPT_HASH_HANDLE, BCryptDestroyHash>;

// CreateFileW fails with INVALID_HANDLE_VALUE, not null
inline UniqueFile adopt_file(HANDLE file) noexcept {
    return UniqueFile(file == INVALID_HANDLE_VALUE ? nullptr : file);
}

}
