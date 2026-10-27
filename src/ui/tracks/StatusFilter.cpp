#include "ui/tracks/StatusFilter.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include <utility>

#include "ui/tracks/StatusTabText.h"

namespace westie::ui {

namespace {

constexpr std::array Tabs = {
    StatusTab::All, StatusTab::Synced, StatusTab::Waiting, StatusTab::NeedsReview, StatusTab::Failed, StatusTab::Duplicates, StatusTab::Removed, StatusTab::Kept,
};

bool always_shown(StatusTab tab) {
    return tab != StatusTab::Removed && tab != StatusTab::Kept && tab != StatusTab::Duplicates;
}

}

StatusFilter::StatusFilter(HWND parent) : tabs_(parent) {
    show();
}

HWND StatusFilter::hwnd() const noexcept {
    return tabs_.hwnd();
}

int StatusFilter::height() const noexcept {
    return tabs_.height();
}

void StatusFilter::apply_dpi(framework::Dpi dpi, const framework::MessageFont& font) {
    tabs_.apply_dpi(dpi, font);
}

StatusTab StatusFilter::selected() const noexcept {
    return selected_;
}

void StatusFilter::set_counts(const StatusCounts& counts) {
    counts_ = counts;
    show();
}

void StatusFilter::show_duplicates(bool shown) {
    duplicates_shown_ = shown;
    if (!shown && selected_ == StatusTab::Duplicates) {
        selected_ = StatusTab::All;
    }
    show();
}

void StatusFilter::reset() {
    selected_ = StatusTab::All;
    show();
}

void StatusFilter::on_selection_change() {
    int index = tabs_.selected();
    if (index >= 0 && std::cmp_less(index, shown_.size())) {
        selected_ = shown_[index];
    }
    show();
}

void StatusFilter::show() {
    shown_.clear();
    std::ranges::copy_if(Tabs, std::back_inserter(shown_), [this](StatusTab tab) {
        if (tab == StatusTab::Duplicates) {
            return duplicates_shown_;
        }
        return always_shown(tab) || status_count(counts_, tab) != 0 || tab == selected_;
    });
    std::vector<std::wstring> texts;
    for (StatusTab tab : shown_) {
        texts.push_back(status_tab_text(counts_, tab));
    }
    auto selected = std::ranges::find(shown_, selected_) - shown_.begin();
    tabs_.set_tabs(texts, static_cast<int>(selected));
}

}
