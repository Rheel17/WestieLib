#include "ui/framework/GdiPlusImage.h"

#include <utility>

#include <objidl.h>
#include <gdiplus.h>

namespace westie::ui::framework {

namespace {

struct StreamReleaser {
    void operator()(IStream* stream) const noexcept { stream->Release(); }
};

using UniqueStream = std::unique_ptr<IStream, StreamReleaser>;

UniqueStream stream_of(std::string_view bytes) {
    IStream* stream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream))) {
        return nullptr;
    }
    UniqueStream owned(stream);
    ULONG written = 0;
    if (FAILED(owned->Write(bytes.data(), static_cast<ULONG>(bytes.size()), &written)) || written != bytes.size()) {
        return nullptr;
    }
    return owned;
}

}

GdiPlusImage::GdiPlusImage(std::unique_ptr<Gdiplus::Bitmap> bitmap) : bitmap_(std::move(bitmap)) {}

GdiPlusImage::GdiPlusImage(GdiPlusImage&& other) noexcept = default;

GdiPlusImage& GdiPlusImage::operator=(GdiPlusImage&& other) noexcept = default;

GdiPlusImage::~GdiPlusImage() = default;

std::optional<GdiPlusImage> GdiPlusImage::decode(std::string_view bytes) {
    UniqueStream stream = stream_of(bytes);
    if (!stream) {
        return std::nullopt;
    }
    std::unique_ptr<Gdiplus::Bitmap> decoded(Gdiplus::Bitmap::FromStream(stream.get()));
    if (!decoded || decoded->GetLastStatus() != Gdiplus::Ok) {
        return std::nullopt;
    }
    // GDI+ reads a stream's image only as it draws it, so it is copied into a bitmap of its own before the stream goes
    auto copy = std::make_unique<Gdiplus::Bitmap>(static_cast<INT>(decoded->GetWidth()), static_cast<INT>(decoded->GetHeight()), PixelFormat32bppARGB);
    if (copy->GetLastStatus() != Gdiplus::Ok) {
        return std::nullopt;
    }
    {
        Gdiplus::Graphics graphics(copy.get());
        if (graphics.DrawImage(decoded.get(), 0, 0, static_cast<INT>(decoded->GetWidth()), static_cast<INT>(decoded->GetHeight())) != Gdiplus::Ok) {
            return std::nullopt;
        }
    }
    return GdiPlusImage(std::move(copy));
}

void GdiPlusImage::draw(HDC dc, const RECT& rect) const {
    Gdiplus::Graphics graphics(dc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    graphics.DrawImage(bitmap_.get(), Gdiplus::Rect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top));
}

}
