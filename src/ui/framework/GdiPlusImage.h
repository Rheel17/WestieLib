#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <windows.h>

namespace Gdiplus {
class Bitmap;
}

namespace westie::ui::framework {

// An encoded image, such as a JPEG, decoded by GDI+
class GdiPlusImage {
public:
    // Nothing for bytes that aren't an image GDI+ reads
    static std::optional<GdiPlusImage> decode(std::string_view bytes);

    GdiPlusImage(GdiPlusImage&& other) noexcept;
    GdiPlusImage& operator=(GdiPlusImage&& other) noexcept;
    ~GdiPlusImage();

    void draw(HDC dc, const RECT& rect) const;

private:
    explicit GdiPlusImage(std::unique_ptr<Gdiplus::Bitmap> bitmap);

    std::unique_ptr<Gdiplus::Bitmap> bitmap_;
};

}
