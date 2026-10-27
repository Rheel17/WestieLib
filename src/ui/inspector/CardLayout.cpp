#include "ui/inspector/CardLayout.h"

#include "ui/framework/Size.h"

namespace westie::ui {

namespace {

constexpr int CoverGapDips = 12;
constexpr int SubtitleGapDips = 1;
constexpr int SectionGapDips = 14;
constexpr int RowsGapDips = 7;
constexpr int RowGapDips = 4;
constexpr int KeyWidthDips = 74;
constexpr int KeyGapDips = 8;
constexpr int ButtonsGapDips = 14;
constexpr int ButtonGapDips = 6;

}

CardLayout::CardLayout(framework::Dpi dpi, int width, int line_height, framework::DeferredPositions* positions) :
        dpi_(dpi), width_(width), line_height_(line_height), positions_(positions) {}

int CardLayout::height() const noexcept {
    return bottom_;
}

void CardLayout::cover(HWND cover, int size) {
    int left = (width_ - size) / 2;
    place(cover, {left, bottom_, left + size, bottom_ + size});
    bottom_ += size + dpi_.scale(CoverGapDips);
}

void CardLayout::title(HWND title, int height) {
    place(title, line(0, 0, height));
}

void CardLayout::block(HWND window, int gap, int height) {
    place(window, line(gap, 0, height));
}

void CardLayout::subtitle(HWND subtitle, int indent) {
    place(subtitle, line(dpi_.scale(SubtitleGapDips), indent, line_height_));
}

void CardLayout::section(HWND title) {
    place(title, line(dpi_.scale(SectionGapDips), 0, line_height_));
    first_row_ = true;
}

void CardLayout::row(HWND key, HWND value) {
    framework::Rect row = line(dpi_.scale(first_row_ ? RowsGapDips : RowGapDips), 0, line_height_);
    first_row_ = false;
    place(key, row.take_left(dpi_.scale(KeyWidthDips)));
    row.take_left(dpi_.scale(KeyGapDips));
    place(value, row);
}

void CardLayout::row_if(bool shown, HWND key, HWND value) {
    if (shown) {
        row(key, value);
    } else {
        hide(key);
        hide(value);
    }
}

void CardLayout::hide(HWND window) {
    if (positions_ != nullptr) {
        positions_->place(window, std::nullopt);
    }
}

void CardLayout::buttons(std::initializer_list<const framework::Button*> buttons) {
    int top = bottom_ + dpi_.scale(ButtonsGapDips);
    int left = 0;
    for (const framework::Button* button : buttons) {
        framework::Size size = button->size();
        // A button that doesn't fit starts a new row, unless it's the first of its row
        if (left > 0 && left + size.width > width_) {
            left = 0;
            top = bottom_ + dpi_.scale(ButtonGapDips);
        }
        place(button->hwnd(), {left, top, left + size.width, top + size.height});
        left += size.width + dpi_.scale(ButtonGapDips);
        bottom_ = top + size.height;
    }
}

framework::Rect CardLayout::line(int gap, int indent, int height) {
    int top = bottom_ + gap;
    bottom_ = top + height;
    return {indent, top, width_, bottom_};
}

void CardLayout::place(HWND window, const framework::Rect& rect) {
    if (positions_ != nullptr) {
        positions_->place(window, rect);
    }
}

}
