#include "ui/framework/Spinner.h"

#include <format>
#include <stdexcept>
#include <string_view>

#include <objidl.h>
#include <gdiplus.h>

namespace westie::ui::framework {

namespace {

// The progress ring's measures in its 16 px glyph, as in the mockup's spin()
constexpr float GlyphSize = 16.0f;
constexpr float Radius = 5.6f;
constexpr float StrokeWidth = 2.4f;
constexpr float ArcSweep = 90.0f;
constexpr float FullTurn = 360.0f;
// GDI+ measures angles clockwise from 3 o'clock
constexpr float TwelveOClock = -90.0f;
constexpr Gdiplus::ARGB TrackColor = 0xFFD6D6D6;
constexpr Gdiplus::ARGB ArcColor = 0xFF0078D7;

void check(Gdiplus::Status status, std::string_view operation) {
    if (status != Gdiplus::Ok) {
        throw std::runtime_error(std::format("{} failed with GDI+ status {}", operation, static_cast<int>(status)));
    }
}

}

void paint_spinner(HDC dc, int x, int y, int size, double turn) {
    Gdiplus::Graphics graphics(dc);
    check(graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias), "SetSmoothingMode");
    // Coordinates on pixel edges, as in SVG, so the circle sits where the glyph's would
    check(graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf), "SetPixelOffsetMode");

    float scale = static_cast<float>(size) / GlyphSize;
    float radius = Radius * scale;
    Gdiplus::RectF circle(static_cast<float>(x) + size / 2.0f - radius, static_cast<float>(y) + size / 2.0f - radius, 2 * radius, 2 * radius);
    Gdiplus::Pen track(Gdiplus::Color(TrackColor), StrokeWidth * scale);
    Gdiplus::Pen arc(Gdiplus::Color(ArcColor), StrokeWidth * scale);
    check(graphics.DrawEllipse(&track, circle), "DrawEllipse");
    check(graphics.DrawArc(&arc, circle, TwelveOClock + FullTurn * static_cast<float>(turn), ArcSweep), "DrawArc");
}

}
