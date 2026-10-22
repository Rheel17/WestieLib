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
constexpr wchar_t LogHeightValue[] = L"LogHeightDips";
constexpr wchar_t LogVisibleValue[] = L"LogVisible";

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

    PanelState defaults;
    return WindowState{
        .placement =
                {
                    .normal_bounds = {*left, *top, *right, *bottom},
                    .maximized = read_int(*key, MaximizedValue).value_or(0) != 0,
                },
        .panels = {
            .playlists_dips = read_int(*key, PlaylistsWidthValue).value_or(defaults.playlists_dips),
            .inspector_dips = read_int(*key, InspectorWidthValue).value_or(defaults.inspector_dips),
            .log_dips = read_int(*key, LogHeightValue).value_or(defaults.log_dips),
            .log_visible = read_int(*key, LogVisibleValue).value_or(defaults.log_visible) != 0,
        },
    };
}

void save_window_state(const WindowState& state) {
    platform::win32::RegistryKey key = platform::win32::RegistryKey::create(HKEY_CURRENT_USER, KeyPath);
    const framework::Rect& bounds = state.placement.normal_bounds;
    write_int(key, LeftValue, bounds.left);
    write_int(key, TopValue, bounds.top);
    write_int(key, RightValue, bounds.right);
    write_int(key, BottomValue, bounds.bottom);
    write_int(key, MaximizedValue, state.placement.maximized);
    write_int(key, PlaylistsWidthValue, state.panels.playlists_dips);
    write_int(key, InspectorWidthValue, state.panels.inspector_dips);
    write_int(key, LogHeightValue, state.panels.log_dips);
    write_int(key, LogVisibleValue, state.panels.log_visible);
}

}
