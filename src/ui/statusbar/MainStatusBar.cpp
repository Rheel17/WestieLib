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
// Shares of the bar's width, with their minimums in DIPs; plan/ui.md gives the reasons for each
constexpr framework::PartWidth RunWidthDips{.minimum = 250, .percent = 22};
constexpr framework::PartWidth TracksWidthDips{.minimum = 100, .percent = 10};
constexpr framework::PartWidth LibraryWidthDips{.minimum = 150, .percent = 36};
constexpr int VersionWidthDips = 80;
constexpr int RingSteps = 24;
constexpr int FullPercent = 100;
constexpr const wchar_t* ToolsText = L"Tools";

enum class Part { Run, Tracks, Library, Slack, Tools, Version };

framework::IconResource load_icon(int resource_id, framework::Dpi dpi) {
    return {resource_id, dpi.scale(IconDips)};
}

std::vector<framework::IconResource> load_rings(framework::Dpi dpi) {
    std::vector<framework::IconResource> rings;
    for (int step = 0; step <= RingSteps; ++step) {
        rings.push_back(load_icon(IDI_RING_0 + step, dpi));
    }
    return rings;
}

}

MainStatusBar::MainStatusBar(HWND parent) :
        status_bar_(parent),
        dpi_(framework::Dpi::of(parent)),
        ready_icon_(load_icon(IDI_SYNCED, dpi_)),
        attention_icon_(load_icon(IDI_WARN, dpi_)),
        rings_(load_rings(dpi_)) {
    // A part has to exist before it takes text
    measure_tools();
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
    measure_tools();
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
    work_ = tools_work(board);
    show_tools();
}

void MainStatusBar::draw(const DRAWITEMSTRUCT& item) const {
    status_bar_.draw(item);
}

void MainStatusBar::load_icons() {
    ready_icon_ = load_icon(IDI_SYNCED, dpi_);
    attention_icon_ = load_icon(IDI_WARN, dpi_);
    rings_ = load_rings(dpi_);
}

void MainStatusBar::measure_tools() {
    // As wide as the widest thing it can say, so it never moves while the tools work
    tools_width_ = status_bar_.part_width_for({.text = ToolsText, .icon = &ready_icon_});
    for (const std::wstring& text : every_tools_work_text()) {
        tools_width_ = std::max(tools_width_, status_bar_.part_width_for({.text = ToolsText, .icon = &rings_.back(), .trailing = text}));
    }
}

void MainStatusBar::show_tools() {
    set_parts();
    framework::PartContent content{.text = ToolsText, .icon = tool_icon()};
    if (work_) {
        content.icon = &ring(work_->percent);
        content.trailing = work_->text;
    }
    status_bar_.set_content(std::to_underlying(Part::Tools), std::move(content));
    status_bar_.set_tip(std::to_underlying(Part::Tools), tools_tip_);
}

void MainStatusBar::set_parts() {
    auto scaled = [this](framework::PartWidth dips) { return framework::PartWidth{.minimum = dpi_.scale(dips.minimum), .percent = dips.percent}; };
    status_bar_.set_parts({
        .left = {scaled(RunWidthDips), scaled(TracksWidthDips), scaled(LibraryWidthDips)},
        .right = {{.minimum = tools_width_}, {.minimum = dpi_.scale(VersionWidthDips)}},
        .yielding = std::to_underlying(Part::Library),
    });
}

const framework::IconResource* MainStatusBar::tool_icon() const noexcept {
    switch (tool_mark_) {
    case ToolMark::Ready:     return &ready_icon_;
    case ToolMark::Working:   return &ring(0);
    case ToolMark::Attention: return &attention_icon_;
    }
    return nullptr;
}

const framework::IconResource& MainStatusBar::ring(int percent) const noexcept {
    int step = (std::clamp(percent, 0, FullPercent) * RingSteps + FullPercent / 2) / FullPercent;
    return rings_[step];
}

}
