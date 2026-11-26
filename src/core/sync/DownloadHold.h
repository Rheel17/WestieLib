#pragma once

#include <functional>

namespace westie::core {

class DownloadHold {
public:
    explicit DownloadHold(std::function<void()> release);
    DownloadHold(DownloadHold&& other) noexcept;
    ~DownloadHold();

    DownloadHold(const DownloadHold&) = delete;
    DownloadHold& operator=(const DownloadHold&) = delete;
    DownloadHold& operator=(DownloadHold&&) = delete;

private:
    std::function<void()> release_;
};

}
