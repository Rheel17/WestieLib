#include "ui/framework/MessageFont.h"

#include <algorithm>

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

namespace {

constexpr int TitleSizePercent = 125;
constexpr int HeadingSizePercent = 150;
// Consolas looks as large as Segoe UI at 11/12 of its size
constexpr int CodeSizeTwelfths = 11;
constexpr int Twelfths = 12;
constexpr wchar_t CodeFace[] = L"Consolas";

platform::win32::UniqueFont create_font(Dpi dpi, FontStyle style) {
    NONCLIENTMETRICSW metrics{};
    metrics.cbSize = sizeof(metrics);
    platform::win32::check(SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, dpi.value()), "SystemParametersInfoForDpi");
    if (style == FontStyle::Semibold || style == FontStyle::Title || style == FontStyle::Heading) {
        metrics.lfMessageFont.lfWeight = FW_SEMIBOLD;
    }
    if (style == FontStyle::Code) {
        metrics.lfMessageFont.lfHeight = MulDiv(metrics.lfMessageFont.lfHeight, CodeSizeTwelfths, Twelfths);
        metrics.lfMessageFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
        std::ranges::copy(CodeFace, metrics.lfMessageFont.lfFaceName);
    }
    if (style == FontStyle::Title) {
        metrics.lfMessageFont.lfHeight = MulDiv(metrics.lfMessageFont.lfHeight, TitleSizePercent, 100);
    }
    if (style == FontStyle::Heading) {
        metrics.lfMessageFont.lfHeight = MulDiv(metrics.lfMessageFont.lfHeight, HeadingSizePercent, 100);
    }
    return platform::win32::UniqueFont(platform::win32::check(CreateFontIndirectW(&metrics.lfMessageFont), "CreateFontIndirectW"));
}

int measure_line_height(HFONT font) {
    platform::win32::UniqueMemoryDc dc(platform::win32::check(CreateCompatibleDC(nullptr), "CreateCompatibleDC"));
    SelectObject(dc.get(), font);
    TEXTMETRICW metrics{};
    platform::win32::check(GetTextMetricsW(dc.get(), &metrics), "GetTextMetricsW");
    return metrics.tmHeight;
}

}

MessageFont::MessageFont(Dpi dpi, FontStyle style) : font_(create_font(dpi, style)), line_height_(measure_line_height(font_.get())) {}

HFONT MessageFont::handle() const noexcept {
    return font_.get();
}

int MessageFont::line_height() const noexcept {
    return line_height_;
}

}
