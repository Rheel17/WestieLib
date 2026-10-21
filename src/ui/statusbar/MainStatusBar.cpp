#include "ui/statusbar/MainStatusBar.h"

#include <algorithm>
#include <array>
#include <utility>

#include "Version.h"

namespace westie::ui {

namespace {

constexpr int HeightDips = 23;
constexpr int TextHeightPaddingDips = 7;
constexpr int TextSidePaddingDips = 8;
constexpr std::array LeftPartWidthsDips = {270, 300, 220};
constexpr int VersionWidthDips = 80;

enum class Part { Run, Library, Tools, Slack, Version };

}

MainStatusBar::MainStatusBar(HWND parent) : status_bar_(parent) {
    // A part has to exist before it takes text
    set_parts(framework::Dpi::of(parent));
    status_bar_.set_text(std::to_underlying(Part::Run), L"Ready");
    status_bar_.set_text(std::to_underlying(Part::Version), L"v" WESTIELIB_VERSION_STRING, framework::TextAlignment::Center);
}

HWND MainStatusBar::hwnd() const noexcept {
    return status_bar_.hwnd();
}

int MainStatusBar::height() const noexcept {
    return height_;
}

void MainStatusBar::apply_dpi(framework::Dpi dpi, const framework::MessageFont& font) {
    status_bar_.set_font(font.handle());
    status_bar_.set_text_padding(dpi.scale(TextSidePaddingDips));
    set_parts(dpi);
    height_ = std::max(dpi.scale(HeightDips), font.line_height() + dpi.scale(TextHeightPaddingDips));
}

void MainStatusBar::draw(const DRAWITEMSTRUCT& item) const {
    status_bar_.draw(item);
}

void MainStatusBar::set_parts(framework::Dpi dpi) {
    std::array<int, LeftPartWidthsDips.size()> widths{};
    std::ranges::transform(LeftPartWidthsDips, widths.begin(), [dpi](int dips) { return dpi.scale(dips); });
    status_bar_.set_parts(widths, dpi.scale(VersionWidthDips));
}

}
