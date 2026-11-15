#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <windows.h>

#include "core/matching/VideoCandidates.h"
#include "core/matching/VideoSearch.h"
#include "core/matching/VideoSearchResults.h"
#include "ui/dialogs/LinkLine.h"
#include "ui/dialogs/PlayButton.h"
#include "ui/dialogs/VideoChoice.h"
#include "ui/dialogs/VideoListView.h"
#include "ui/framework/DialogSizing.h"
#include "ui/framework/ModalDialog.h"
#include "ui/framework/Size.h"

namespace westie::ui {

// The title of a video the library has
using LibraryVideoTitle = std::function<std::optional<std::string>(const std::string& video_id)>;

class ChooseVideoDialog final : public framework::ModalDialog {
public:
    static constexpr framework::Size DefaultWindowDips{900, 600};

    ChooseVideoDialog(VideoChoice choice, core::VideoSearch& search, LibraryVideoTitle library_title, framework::Size window_dips);

    // The video id
    std::optional<std::string> choose(HWND owner);
    void show_results(const core::VideoSearchResults& results);
    // The size it was closed at
    framework::Size window_dips() const noexcept;

protected:
    INT_PTR handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    void on_init_dialog();
    void on_command(WPARAM wparam);
    INT_PTR on_notify(NMHDR& header);
    void on_draw_item(const DRAWITEMSTRUCT& item) const;
    void on_dpi_changed();
    void search(std::optional<std::string> query);
    void search_typed();
    void check_link();
    void show_buttons();
    std::optional<std::string> chosen_video() const;
    void play_source() const;
    void play_video(const std::string& video_id) const;
    void accept();

    VideoChoice choice_;
    core::VideoSearch& search_;
    LibraryVideoTitle library_title_;
    framework::Size window_dips_;
    std::optional<framework::DialogSizing> sizing_;
    std::optional<VideoListView> list_;
    std::optional<PlayButton> source_play_;
    std::optional<PlayButton> link_play_;
    int request_ = 0;
    std::vector<core::VideoCandidate> candidates_;
    std::optional<std::string> link_;
    LinkLine link_line_;
    std::optional<std::string> chosen_;
};

}
