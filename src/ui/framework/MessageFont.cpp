#include "ui/framework/MessageFont.h"

#include "platform/win32/Win32Error.h"

namespace westie::ui::framework {

namespace {

platform::win32::UniqueFont create_font(Dpi dpi, FontStyle style) {
    NONCLIENTMETRICSW metrics{};
    metrics.cbSize = sizeof(metrics);
    platform::win32::check(SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, dpi.value()), "SystemParametersInfoForDpi");
    if (style == FontStyle::Semibold) {
        metrics.lfMessageFont.lfWeight = FW_SEMIBOLD;
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
