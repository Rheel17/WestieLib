#include "core/download/CoverArt.h"

#include <cstdint>
#include <format>
#include <stdexcept>
#include <string_view>

#include "core/json/Utf8Path.h"

namespace westie::core {

namespace {

constexpr std::wstring_view CoverFile = L"cover.jpg";
// A video's thumbnail is wider than it is tall, and a cover is square
constexpr std::string_view SquareCrop = "crop=min(iw\\,ih):min(iw\\,ih)";
constexpr std::string_view JpegQuality = "2";

}

CoverArt::CoverArt(platform::HttpClient& http, const Ffmpeg& ffmpeg, const platform::FileOps& files) : http_(http), ffmpeg_(ffmpeg), files_(files) {}

std::string CoverArt::fetch(
        const std::string& cover_url, const std::optional<std::filesystem::path>& thumbnail, const std::filesystem::path& work, const LogSubject& subject,
        const std::stop_token& stop
) const {
    std::filesystem::path cover = work / CoverFile;
    if (!cover_url.empty()) {
        http_.download(cover_url, cover, [](std::uint64_t) {}, stop);
    } else {
        if (!thumbnail) {
            throw std::runtime_error("The video has no thumbnail");
        }
        platform::ProcessResult result = ffmpeg_.run(
                {"-y", "-i", to_utf8(*thumbnail), "-vf", std::string(SquareCrop), "-q:v", std::string(JpegQuality), to_utf8(cover)}, subject, stop
        );
        if (result.exit_code != 0) {
            throw std::runtime_error(std::format("ffmpeg exited with code {}", result.exit_code));
        }
    }
    std::optional<std::string> jpeg = files_.read_file(cover);
    if (!jpeg || jpeg->empty()) {
        throw std::runtime_error("The cover is empty");
    }
    return std::move(*jpeg);
}

}
