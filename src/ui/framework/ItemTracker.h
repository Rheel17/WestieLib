#pragma once

#include <cstddef>
#include <optional>

#include <windows.h>

namespace westie::ui::framework {

class ItemTracker {
public:
    std::optional<std::size_t> hot() const noexcept;
    std::optional<std::size_t> pressed() const noexcept;
    bool move(HWND window, std::optional<std::size_t> item);
    bool leave() noexcept;
    bool press(HWND window, std::optional<std::size_t> item);
    std::optional<std::size_t> release(HWND window, std::optional<std::size_t> item);
    bool cancel() noexcept;

private:
    static void track_leave(HWND window);

    std::optional<std::size_t> hot_;
    std::optional<std::size_t> pressed_;
};

}
