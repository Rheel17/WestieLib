#include "ui/inspector/TrackPages.h"

#include <algorithm>

#include "core/sources/SourcePage.h"
#include "core/sources/SourceType.h"
#include "platform/win32/Utf16.h"

namespace westie::ui {

TrackPages track_pages(const TrackDetails& details) {
    TrackPages pages;
    if (details.audio) {
        pages.youtube_music = details.audio->url;
    }
    if (core::source_type_of_key(details.row.key) == core::SourceType::Spotify) {
        if (std::optional<std::string> url = core::source_page_url(details.row.key)) {
            pages.spotify = platform::win32::to_utf16(*url);
        }
    } else if (auto source = std::ranges::find(details.sources, core::SourceType::Spotify, &TrackSource::type); source != details.sources.end()) {
        pages.spotify = source->url;
    }
    return pages;
}

}
