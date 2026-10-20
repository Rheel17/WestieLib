#include "ui/main/WindowState.h"

#include <windows.h>

#include "platform/win32/RegistryKey.h"

namespace westie::ui {

namespace {

constexpr wchar_t KeyPath[] = L"Software\\WestieLib\\Window";
constexpr wchar_t LeftValue[] = L"Left";
constexpr wchar_t TopValue[] = L"Top";
constexpr wchar_t RightValue[] = L"Right";
constexpr wchar_t BottomValue[] = L"Bottom";
constexpr wchar_t MaximizedValue[] = L"Maximized";
constexpr wchar_t PlaylistsWidthValue[] = L"PlaylistsWidthDips";
constexpr wchar_t InspectorWidthValue[] = L"InspectorWidthDips";

std::optional<int> read_int(const platform::win32::RegistryKey& key, const wchar_t* name) {
    return key.read_dword(name).transform([](DWORD value) { return static_cast<int>(value); });
}

void write_int(platform::win32::RegistryKey& key, const wchar_t* name, int value) {
    key.write_dword(name, static_cast<DWORD>(value));
}

}

std::optional<WindowState> load_window_state() {
    std::optional<platform::win32::RegistryKey> key = platform::win32::RegistryKey::open(HKEY_CURRENT_USER, KeyPath);
    if (!key) {
        return std::nullopt;
    }

    std::optional<int> left = read_int(*key, LeftValue);
    std::optional<int> top = read_int(*key, TopValue);
    std::optional<int> right = read_int(*key, RightValue);
    std::optional<int> bottom = read_int(*key, BottomValue);
    if (!left || !top || !right || !bottom) {
        return std::nullopt;
    }

    PanelWidths defaults;
    return WindowState{
        .normal_bounds = {*left, *top, *right, *bottom},
        .maximized = read_int(*key, MaximizedValue).value_or(0) != 0,
        .panel_widths = {
            .playlists_dips = read_int(*key, PlaylistsWidthValue).value_or(defaults.playlists_dips),
            .inspector_dips = read_int(*key, InspectorWidthValue).value_or(defaults.inspector_dips),
        },
    };
}

void save_window_state(const WindowState& state) {
    platform::win32::RegistryKey key = platform::win32::RegistryKey::create(HKEY_CURRENT_USER, KeyPath);
    write_int(key, LeftValue, state.normal_bounds.left);
    write_int(key, TopValue, state.normal_bounds.top);
    write_int(key, RightValue, state.normal_bounds.right);
    write_int(key, BottomValue, state.normal_bounds.bottom);
    write_int(key, MaximizedValue, state.maximized);
    write_int(key, PlaylistsWidthValue, state.panel_widths.playlists_dips);
    write_int(key, InspectorWidthValue, state.panel_widths.inspector_dips);
}

}
