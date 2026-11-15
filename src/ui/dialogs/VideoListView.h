#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <windows.h>
#include <commctrl.h>

#include "core/matching/VideoCandidates.h"
#include "ui/dialogs/RowPlayButtons.h"
#include "ui/framework/Control.h"

namespace westie::ui {

class VideoListView final : public framework::Control {
public:
    // Gets the row whose play button was clicked
    VideoListView(HWND list, std::function<void(std::size_t)> on_play);

    // A length further off the source's than a few seconds shows in amber
    void show(const std::vector<core::VideoCandidate>& candidates, std::chrono::milliseconds source_duration);
    // What it reads while it has no rows
    void set_empty_text(std::wstring text);
    std::optional<std::size_t> selected() const;
    LRESULT on_custom_draw(NMLVCUSTOMDRAW& draw) const;
    void on_empty_markup(NMLVEMPTYMARKUP& markup) const;

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void add_columns();
    void fit_columns();
    void set_text(int item, int column, const std::wstring& text);

    RowPlayButtons play_;
    std::wstring empty_text_;
    std::vector<bool> length_off_;
};

}
