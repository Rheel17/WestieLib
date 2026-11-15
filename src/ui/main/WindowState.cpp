#include "ui/main/WindowState.h"

#include <array>
#include <utility>

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
constexpr wchar_t LogLevelValue[] = L"LogLevel";
constexpr wchar_t LogScopeValue[] = L"LogScope";
constexpr wchar_t ChooseMatchWidthValue[] = L"ChooseMatchWidthDips";
constexpr wchar_t ChooseMatchHeightValue[] = L"ChooseMatchHeightDips";

struct ColumnValue {
    TrackColumn column = TrackColumn::Title;
    const wchar_t* name = nullptr;
};

constexpr std::array ColumnValues = {
    ColumnValue{TrackColumn::Number, L"NumberColumnWidthDips"},
    ColumnValue{TrackColumn::Status, L"StatusColumnWidthDips"},
    ColumnValue{TrackColumn::Title, L"TitleColumnWidthDips"},
    ColumnValue{TrackColumn::Artists, L"ArtistsColumnWidthDips"},
    ColumnValue{TrackColumn::Album, L"AlbumColumnWidthDips"},
    ColumnValue{TrackColumn::Playlists, L"PlaylistsColumnWidthDips"},
    ColumnValue{TrackColumn::Time, L"TimeColumnWidthDips"},
    ColumnValue{TrackColumn::Source, L"SourceColumnWidthDips"},
    ColumnValue{TrackColumn::Match, L"MatchColumnWidthDips"},
    ColumnValue{TrackColumn::File, L"FileColumnWidthDips"},
};

std::optional<int> read_int(const platform::win32::RegistryKey& key, const wchar_t* name) {
    return key.read_dword(name).transform([](DWORD value) { return static_cast<int>(value); });
}

void write_int(platform::win32::RegistryKey& key, const wchar_t* name, int value) {
    key.write_dword(name, static_cast<DWORD>(value));
}

std::map<TrackColumn, int> read_columns(const platform::win32::RegistryKey& key) {
    std::map<TrackColumn, int> widths;
    for (const ColumnValue& value : ColumnValues) {
        if (std::optional<int> width = read_int(key, value.name)) {
            widths[value.column] = *width;
        }
    }
    return widths;
}

// A value from another version that names no level or scope keeps the default
LogView read_log_view(const platform::win32::RegistryKey& key) {
    LogView view;
    if (std::optional<int> level = read_int(key, LogLevelValue); level && *level >= 0 && *level <= std::to_underlying(core::LogLevel::Error)) {
        view.least = static_cast<core::LogLevel>(*level);
    }
    if (std::optional<int> scope = read_int(key, LogScopeValue); scope && *scope >= 0 && *scope <= std::to_underlying(LogScope::Track)) {
        view.scope = static_cast<LogScope>(*scope);
    }
    return view;
}

std::optional<framework::Size> read_size(const platform::win32::RegistryKey& key, const wchar_t* width_name, const wchar_t* height_name) {
    std::optional<int> width = read_int(key, width_name);
    std::optional<int> height = read_int(key, height_name);
    if (!width || !height) {
        return std::nullopt;
    }
    return framework::Size{*width, *height};
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
        .column_dips = read_columns(*key),
        .log_view = read_log_view(*key),
        .choose_match_dips = read_size(*key, ChooseMatchWidthValue, ChooseMatchHeightValue),
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
    write_int(key, LogLevelValue, std::to_underlying(state.log_view.least));
    write_int(key, LogScopeValue, std::to_underlying(state.log_view.scope));
    if (state.choose_match_dips) {
        write_int(key, ChooseMatchWidthValue, state.choose_match_dips->width);
        write_int(key, ChooseMatchHeightValue, state.choose_match_dips->height);
    }
    for (const ColumnValue& value : ColumnValues) {
        if (auto width = state.column_dips.find(value.column); width != state.column_dips.end()) {
            write_int(key, value.name, width->second);
        }
    }
}

}
