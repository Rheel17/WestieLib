#pragma once

#include <optional>

#include <windows.h>

#include "platform/win32/UniqueHandle.h"

namespace westie::platform::win32 {

class RegistryKey {
public:
    static std::optional<RegistryKey> open(HKEY parent, const wchar_t* path);
    static RegistryKey create(HKEY parent, const wchar_t* path);

    std::optional<DWORD> read_dword(const wchar_t* name) const;
    void write_dword(const wchar_t* name, DWORD value);

private:
    explicit RegistryKey(HKEY key);

    UniqueKey key_;
};

}
