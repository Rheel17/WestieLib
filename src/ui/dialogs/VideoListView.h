#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <windows.h>
#include <commctrl.h>

#include "core/matching/VideoCandidates.h"

namespace westie::ui {

class VideoListView {
public:
    explicit VideoListView(HWND list);

    // A length further off the source's than a few seconds shows in amber
    void show(const std::vector<core::VideoCandidate>& candidates, std::chrono::milliseconds source_duration);
    // What it reads while it has no rows
    void set_empty_text(std::wstring text);
    std::optional<std::size_t> selected() const;
    LRESULT on_custom_draw(NMLVCUSTOMDRAW& draw) const;
    void on_empty_markup(NMLVEMPTYMARKUP& markup) const;

private:
    void add_columns();
    void set_text(int item, int column, const std::wstring& text);

    HWND list_ = nullptr;
    std::wstring empty_text_;
    std::vector<bool> length_off_;
};

}
