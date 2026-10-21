#include "ui/welcome/LibraryFolderField.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "platform/win32/Win32Error.h"
#include "ui/dialogs/FolderPicker.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Rect.h"
#include "ui/framework/Size.h"
#include "ui/welcome/LibraryFolderCheck.h"
#include "ui/welcome/LibraryFolderText.h"

namespace westie::ui {

namespace {

constexpr int ButtonGapDips = 8;
constexpr int HintGapDips = 4;
constexpr int HintLineDips = 16;

}

LibraryFolderField::LibraryFolderField(HWND parent, std::filesystem::path default_folder, std::function<void(const std::filesystem::path&)> on_change) :
        Panel(parent, L"Library folder"),
        default_folder_(std::move(default_folder)),
        on_change_(std::move(on_change)),
        edit_(hwnd()),
        browse_(hwnd(), {.text = L"Browse…"}),
        hint_(hwnd(), L"", {.wrap = true}) {}

int LibraryFolderField::height_for(int width) const {
    return row_height() + framework::Dpi::of(hwnd()).scale(HintGapDips) + hint_height(width);
}

void LibraryFolderField::apply_dpi(framework::Dpi dpi, const framework::MessageFont& font) {
    edit_.apply_dpi(dpi, font);
    browse_.apply_dpi(dpi, font);
    hint_.set_font(font.handle());
    layout();
}

void LibraryFolderField::set_folder(const std::filesystem::path& folder) {
    folder_ = folder;
    edit_.set_text(folder.native());
    show_hint();
}

void LibraryFolderField::commit() {
    LibraryFolderCheck check = check_library_folder(edit_.text());
    if (!check.folder) {
        return;
    }
    edit_.set_text(check.folder->native());
    if (*check.folder != folder_) {
        folder_ = std::move(*check.folder);
        on_change_(folder_);
    }
}

LRESULT LibraryFolderField::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:    layout(); return 0;
    case WM_COMMAND: on_command(wparam, lparam); return 0;
    default:         return Panel::handle_message(message, wparam, lparam);
    }
}

int LibraryFolderField::row_height() const noexcept {
    return std::max(edit_.height(), browse_.size().height);
}

int LibraryFolderField::hint_height(int width) const {
    // Room for the longest hint, so nothing moves when the hint changes
    int height = framework::Dpi::of(hwnd()).scale(HintLineDips);
    for (FolderHint hint : ShownFolderHints) {
        height = std::max(height, hint_.height_for(hint_text(hint, default_folder_), width));
    }
    return height;
}

void LibraryFolderField::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::Rect area = framework::Rect::from_rect(client);
    framework::Rect row = area.take_top(row_height());
    area.take_top(framework::Dpi::of(hwnd()).scale(HintGapDips));
    framework::Size button = browse_.size();
    framework::Rect browse = row.take_right(button.width);
    row.take_right(framework::Dpi::of(hwnd()).scale(ButtonGapDips));
    int edit_top = row.top + (row.height() - edit_.height()) / 2;
    int button_top = row.top + (row.height() - button.height) / 2;

    framework::DeferredPositions positions;
    positions.place(edit_.hwnd(), framework::Rect{row.left, edit_top, row.right, edit_top + edit_.height()});
    positions.place(browse_.hwnd(), framework::Rect{browse.left, button_top, browse.right, button_top + button.height});
    positions.place(hint_.hwnd(), area);
    positions.apply();
}

void LibraryFolderField::on_command(WPARAM wparam, LPARAM lparam) {
    auto control = reinterpret_cast<HWND>(lparam);
    if (control == edit_.hwnd() && HIWORD(wparam) == EN_CHANGE) {
        show_hint();
    } else if (control == edit_.hwnd() && HIWORD(wparam) == EN_KILLFOCUS) {
        commit();
    } else if (control == browse_.hwnd() && HIWORD(wparam) == BN_CLICKED) {
        browse();
    }
}

void LibraryFolderField::show_hint() {
    FolderHint hint = check_library_folder(edit_.text()).hint;
    hint_.set_text(hint_text(hint, default_folder_));
    hint_.set_color(is_problem(hint) ? framework::TextColor::Error : framework::TextColor::Gray);
}

void LibraryFolderField::browse() {
    std::filesystem::path start = parse_library_folder(edit_.text()).value_or(folder_);
    if (std::optional<std::filesystem::path> folder = pick_folder(GetAncestor(hwnd(), GA_ROOT), L"Library folder", start)) {
        edit_.set_text(folder->native());
        commit();
    }
}

}
