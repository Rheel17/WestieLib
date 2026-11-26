#include "core/sync/DownloadHold.h"

#include <utility>

namespace westie::core {

DownloadHold::DownloadHold(std::function<void()> release) : release_(std::move(release)) {}

DownloadHold::DownloadHold(DownloadHold&& other) noexcept : release_(std::exchange(other.release_, nullptr)) {}

DownloadHold::~DownloadHold() {
    if (release_) {
        release_();
    }
}

}
