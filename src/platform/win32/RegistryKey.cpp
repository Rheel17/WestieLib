#include "platform/win32/RegistryKey.h"

#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

RegistryKey::RegistryKey(HKEY key) : key_(key) {}

std::optional<RegistryKey> RegistryKey::open(HKEY parent, const wchar_t* path) {
    HKEY key;
    LSTATUS status = RegOpenKeyExW(parent, path, 0, KEY_READ, &key);
    if (status == ERROR_FILE_NOT_FOUND) {
        return std::nullopt;
    }
    check_status(status, "RegOpenKeyExW");
    return RegistryKey(key);
}

RegistryKey RegistryKey::create(HKEY parent, const wchar_t* path) {
    HKEY key;
    check_status(RegCreateKeyExW(parent, path, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, nullptr, &key, nullptr), "RegCreateKeyExW");
    return RegistryKey(key);
}

std::optional<DWORD> RegistryKey::read_dword(const wchar_t* name) const {
    DWORD value;
    DWORD size = sizeof(value);
    LSTATUS status = RegGetValueW(key_.get(), nullptr, name, RRF_RT_REG_DWORD, nullptr, &value, &size);
    if (status == ERROR_FILE_NOT_FOUND) {
        return std::nullopt;
    }
    check_status(status, "RegGetValueW");
    return value;
}

void RegistryKey::write_dword(const wchar_t* name, DWORD value) {
    check_status(RegSetValueExW(key_.get(), name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value)), "RegSetValueExW");
}

}
