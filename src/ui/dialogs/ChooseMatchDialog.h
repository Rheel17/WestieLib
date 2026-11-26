#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <windows.h>

#include "core/matching/MatchCandidates.h"
#include "core/matching/CandidateSearch.h"
#include "core/matching/CandidateSearchResults.h"
#include "ui/dialogs/LinkLine.h"
#include "ui/dialogs/PlayButton.h"
#include "ui/dialogs/MatchChoice.h"
#include "ui/dialogs/CandidateListView.h"
#include "ui/framework/DialogSizing.h"
#include "ui/framework/ModalDialog.h"
#include "ui/framework/Size.h"

namespace westie::ui {

// The title of a YouTube Music id the library has
using LibraryResultTitle = std::function<std::optional<std::string>(const std::string& ytm_id)>;

struct ChooseMatchOptions {
    framework::Size window_dips{};
    // The check box's state as it opens
    bool open_by_itself = true;
    // Another track waits, so Next shows
    bool next = false;
};

enum class ChooseMatchEnd { Chosen, Next, Cancelled };

struct ChooseMatchOutcome {
    ChooseMatchEnd end = ChooseMatchEnd::Cancelled;
    // Set when chosen
    std::string ytm_id{};
    // The check box's state as it closed
    bool open_by_itself = true;
};

class ChooseMatchDialog final : public framework::ModalDialog {
public:
    static constexpr framework::Size DefaultWindowDips{900, 600};

    ChooseMatchDialog(MatchChoice choice, core::CandidateSearch& search, LibraryResultTitle library_title, const ChooseMatchOptions& options);

    ChooseMatchOutcome choose(HWND owner);
    void show_results(const core::CandidateSearchResults& results);
    // As other tracks start to wait while it is open
    void show_next();
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
    std::optional<std::string> chosen_ytm_id() const;
    void play_source() const;
    void play_result(const std::string& ytm_id) const;
    void accept();

    MatchChoice choice_;
    core::CandidateSearch& search_;
    LibraryResultTitle library_title_;
    framework::Size window_dips_;
    bool open_by_itself_ = true;
    bool next_ = false;
    std::optional<framework::DialogSizing> sizing_;
    std::optional<CandidateListView> list_;
    std::optional<PlayButton> source_play_;
    std::optional<PlayButton> link_play_;
    int request_ = 0;
    std::vector<core::MatchCandidate> candidates_;
    std::optional<std::string> link_;
    LinkLine link_line_;
    std::optional<std::string> chosen_;
};

}
