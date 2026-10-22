#include "ui/statusbar/MainStatusBar.h"

#include <algorithm>
#include <utility>

#include "ResourceIds.h"
#include "Version.h"
#include "ui/TrackCountText.h"

namespace westie::ui {

namespace {

constexpr int HeightDips = 23;
constexpr int TextHeightPaddingDips = 7;
constexpr int TextSidePaddingDips = 8;
constexpr int GapDips = 7;
constexpr int IconDips = 12;
constexpr framework::Size ProgressBarDips{60, 16};
// Shares of the bar's width, with their minimums in DIPs; plan/ui.md gives the reasons for each
constexpr framework::PartWidth RunWidthDips{.minimum = 250, .percent = 22};
constexpr framework::PartWidth TracksWidthDips{.minimum = 100, .percent = 10};
constexpr framework::PartWidth LibraryWidthDips{.minimum = 150, .percent = 36};
constexpr framework::PartWidth ToolsWidthDips{.minimum = 100, .percent = 8};
constexpr int VersionWidthDips = 80;

enum class Part { Run, Tracks, Library, Slack, Progress, Tools, Version };

framework::IconResource load_icon(int resource_id, framework::Dpi dpi) {
    return {resource_id, dpi.scale(IconDips)};
}

}

MainStatusBar::MainStatusBar(HWND parent) :
        status_bar_(parent),
        progress_bar_(status_bar_.hwnd()),
        dpi_(framework::Dpi::of(parent)),
        ready_icon_(load_icon(IDI_SYNCED, dpi_)),
        working_icon_(load_icon(IDI_SYNCING, dpi_)),
        attention_icon_(load_icon(IDI_WARN, dpi_)) {
    // A part has to exist before it takes text
    set_parts();
    status_bar_.set_text(std::to_underlying(Part::Run), L"Ready");
    status_bar_.set_text(std::to_underlying(Part::Version), L"v" WESTIELIB_VERSION_STRING, {.alignment = framework::TextAlignment::Center});
    show_tools();
}

HWND MainStatusBar::hwnd() const noexcept {
    return status_bar_.hwnd();
}

int MainStatusBar::height() const noexcept {
    return height_;
}

void MainStatusBar::apply_dpi(framework::Dpi dpi, const framework::MessageFont& font) {
    dpi_ = dpi;
    status_bar_.set_font(font.handle());
    status_bar_.set_spacing(dpi.scale(TextSidePaddingDips), dpi.scale(GapDips));
    load_icons();
    height_ = std::max(dpi.scale(HeightDips), font.line_height() + dpi.scale(TextHeightPaddingDips));
    // The parts are measured in the new font
    show_tools();
}

void MainStatusBar::set_track_count(int count) {
    status_bar_.set_text(std::to_underlying(Part::Tracks), track_count_text(count));
}

void MainStatusBar::set_library_folder(const std::filesystem::path& folder) {
    status_bar_.set_text(std::to_underlying(Part::Library), folder.wstring(), {.ellipsis = framework::TextEllipsis::Path});
}

void MainStatusBar::set_tools(const ToolBoard& board) {
    tool_mark_ = board.mark();
    tools_tip_ = tools_tooltip_text(board);
    progress_ = tools_progress(board);
    show_tools();
}

void MainStatusBar::draw(const DRAWITEMSTRUCT& item) const {
    status_bar_.draw(item);
}

void MainStatusBar::load_icons() {
    ready_icon_ = load_icon(IDI_SYNCED, dpi_);
    working_icon_ = load_icon(IDI_SYNCING, dpi_);
    attention_icon_ = load_icon(IDI_WARN, dpi_);
}

void MainStatusBar::show_tools() {
    set_parts();
    status_bar_.set_text(std::to_underlying(Part::Tools), L"Tools", {.icon = &tool_icon()});
    status_bar_.set_tip(std::to_underlying(Part::Tools), tools_tip_);
    status_bar_.set_text(std::to_underlying(Part::Progress), progress_ ? progress_->text : L"");

    std::optional<framework::Size> bar_size;
    if (progress_ && progress_->percent) {
        progress_bar_.set_percent(*progress_->percent);
        bar_size = dpi_.scale(ProgressBarDips);
    }
    status_bar_.embed(std::to_underlying(Part::Progress), progress_bar_.hwnd(), bar_size);
}

void MainStatusBar::set_parts() {
    auto scaled = [this](framework::PartWidth dips) { return framework::PartWidth{.minimum = dpi_.scale(dips.minimum), .percent = dips.percent}; };
    status_bar_.set_parts({
        .left = {scaled(RunWidthDips), scaled(TracksWidthDips), scaled(LibraryWidthDips)},
        .right = {{.minimum = progress_width()}, scaled(ToolsWidthDips), {.minimum = dpi_.scale(VersionWidthDips)}},
        .yielding = std::to_underlying(Part::Library),
    });
}

int MainStatusBar::progress_width() const {
    if (!progress_) {
        return 0;
    }
    int width = status_bar_.part_width_for(progress_->text);
    if (progress_->percent) {
        width += dpi_.scale(GapDips) + dpi_.scale(ProgressBarDips.width);
    }
    return width;
}

const framework::IconResource& MainStatusBar::tool_icon() const noexcept {
    switch (tool_mark_) {
    case ToolMark::Ready:     return ready_icon_;
    case ToolMark::Working:   return working_icon_;
    case ToolMark::Attention: return attention_icon_;
    }
    return attention_icon_;
}

}
