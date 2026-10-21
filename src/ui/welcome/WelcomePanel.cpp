#include "ui/welcome/WelcomePanel.h"

#include <algorithm>
#include <utility>

#include "ResourceIds.h"
#include "platform/win32/Win32Error.h"
#include "ui/framework/BufferedPaint.h"
#include "ui/framework/DeferredPositions.h"
#include "ui/framework/Size.h"

namespace westie::ui {

namespace {

constexpr int ColumnWidthDips = 430;
constexpr int MarginDips = 16;
constexpr int IconDips = 40;
constexpr int HeadingGapDips = 12;
constexpr int HeadingLineDips = 24;
constexpr int IntroductionGapDips = 6;
constexpr int LabelGapDips = 18;
constexpr int LabelLineDips = 16;
constexpr int FieldGapDips = 4;
constexpr int ButtonGapDips = 16;

}

WelcomePanel::WelcomePanel(
        HWND parent, const std::filesystem::path& default_library_folder, std::function<void(CommandId)> on_command,
        std::function<void(const std::filesystem::path&)> on_library_folder
) :
        Panel(parent, L"Welcome"),
        on_command_(std::move(on_command)),
        icon_(IDI_APP_ICON, framework::Dpi::of(hwnd()).scale(IconDips)),
        heading_(hwnd(), L"Welcome to WestieLib"),
        introduction_(
                hwnd(),
                L"WestieLib keeps your YouTube Music and Spotify playlists in sync with a folder of tagged audio files. Choose where the files go, then "
                L"add your first playlist.",
                {.color = framework::TextColor::Gray, .wrap = true}
        ),
        folder_label_(hwnd(), L"Library folder"),
        folder_(hwnd(), default_library_folder, std::move(on_library_folder)),
        add_button_(
                hwnd(), {.text = L"Add your first playlist", .command = std::to_underlying(CommandId::AddPlaylist), .icon = IDI_PLUS, .default_button = true}
        ) {}

void WelcomePanel::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    icon_ = framework::IconResource(IDI_APP_ICON, dpi.scale(IconDips));
    heading_.set_font(fonts.heading().handle());
    introduction_.set_font(fonts.regular().handle());
    folder_label_.set_font(fonts.regular().handle());
    heading_height_ = std::max(dpi.scale(HeadingLineDips), fonts.heading().line_height());
    label_height_ = std::max(dpi.scale(LabelLineDips), fonts.regular().line_height());
    folder_.apply_dpi(dpi, fonts.regular());
    add_button_.apply_dpi(dpi, fonts.regular());
    layout();
    platform::win32::check(InvalidateRect(hwnd(), nullptr, FALSE), "InvalidateRect");
}

void WelcomePanel::set_library_folder(const std::filesystem::path& folder) {
    folder_.set_folder(folder);
}

LRESULT WelcomePanel::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:       layout(); return 0;
    // The paint fills the background, so the icon isn't erased first and doesn't blink
    case WM_ERASEBKGND: return TRUE;
    case WM_PAINT:      paint(); return 0;
    case WM_COMMAND:    on_command(wparam); return 0;
    default:            return Panel::handle_message(message, wparam, lparam);
    }
}

void WelcomePanel::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::Rect area = framework::Rect::from_rect(client);
    framework::Dpi dpi = framework::Dpi::of(hwnd());

    int width = std::max(0, std::min(dpi.scale(ColumnWidthDips), area.width() - 2 * dpi.scale(MarginDips)));
    int introduction_height = introduction_.height_for(width);
    int field_height = folder_.height_for(width);
    framework::Size button = add_button_.size();
    int height = icon_.size() + dpi.scale(HeadingGapDips) + heading_height_ + dpi.scale(IntroductionGapDips) + introduction_height +
                 dpi.scale(LabelGapDips) + label_height_ + dpi.scale(FieldGapDips) + field_height + dpi.scale(ButtonGapDips) + button.height;
    int left = (area.width() - width) / 2;
    int top = std::max(dpi.scale(MarginDips), (area.height() - height) / 2);

    // The column's parts from the top, each followed by its gap
    auto take = [&](int part_height, int gap) {
        framework::Rect part{left, top, left + width, top + part_height};
        top = part.bottom + gap;
        return part;
    };
    framework::Rect icon = take(icon_.size(), dpi.scale(HeadingGapDips));
    framework::Rect heading = take(heading_height_, dpi.scale(IntroductionGapDips));
    framework::Rect introduction = take(introduction_height, dpi.scale(LabelGapDips));
    framework::Rect label = take(label_height_, dpi.scale(FieldGapDips));
    framework::Rect field = take(field_height, dpi.scale(ButtonGapDips));
    framework::Rect add = take(button.height, 0);
    icon.right = icon.left + icon_.size();
    add.right = add.left + button.width;

    if (icon != icon_rect_) {
        RECT old_icon = icon_rect_.to_rect();
        RECT new_icon = icon.to_rect();
        platform::win32::check(InvalidateRect(hwnd(), &old_icon, FALSE), "InvalidateRect");
        platform::win32::check(InvalidateRect(hwnd(), &new_icon, FALSE), "InvalidateRect");
        icon_rect_ = icon;
    }

    framework::DeferredPositions positions;
    positions.place(heading_.hwnd(), heading);
    positions.place(introduction_.hwnd(), introduction);
    positions.place(folder_label_.hwnd(), label);
    positions.place(folder_.hwnd(), field);
    positions.place(add_button_.hwnd(), add);
    positions.apply();
}

void WelcomePanel::paint() const {
    framework::BufferedPaint paint(hwnd());
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    FillRect(paint.dc(), &client, GetSysColorBrush(COLOR_WINDOW));
    icon_.draw(paint.dc(), icon_rect_.left, icon_rect_.top);
}

void WelcomePanel::on_command(WPARAM wparam) const {
    if (HIWORD(wparam) == BN_CLICKED) {
        on_command_(static_cast<CommandId>(LOWORD(wparam)));
    }
}

}
