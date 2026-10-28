#include "ui/inspector/SelectionCard.h"

#include <algorithm>
#include <format>

#include "platform/win32/Win32Error.h"
#include "ui/inspector/CardLayout.h"
#include "ui/inspector/TrackStatusText.h"
#include "ui/tracks/StatusTabText.h"

namespace westie::ui {

namespace {

constexpr int LineDips = 16;
constexpr int AllRemovedGapDips = 6;
constexpr IconTextStyle CountStyle{.gap_dips = 4, .line_dips = 16};

}

SelectionCard::SelectionCard(HWND parent) :
        Panel(parent, L"Selection card"),
        title_(hwnd(), CardTitleStyle),
        section_(hwnd(), L"Status"),
        // Needs review counts under the track's status
        rows_{
            status_row(StatusTab::Synced), status_row(StatusTab::Waiting), status_row(StatusTab::Failed), status_row(StatusTab::Removed),
            status_row(StatusTab::Kept),
        },
        all_removed_(hwnd(), L"No longer in any playlist. Their files are still in the library.", {.color = framework::TextColor::Gray, .wrap = true}) {}

int SelectionCard::height_for(int width) const {
    return arrange(width, nullptr);
}

void SelectionCard::apply_dpi(framework::Dpi dpi, const framework::FontSet& fonts) {
    title_.apply_dpi(dpi, fonts.title());
    section_.set_font(fonts.semibold().handle());
    for (StatusRow& row : rows_) {
        row.key.set_font(fonts.regular().handle());
        row.count.apply_dpi(dpi, fonts.regular());
    }
    all_removed_.set_font(fonts.regular().handle());
    line_height_ = std::max(dpi.scale(LineDips), fonts.regular().line_height());
    layout();
}

void SelectionCard::set_contents(const StatusCounts& counts) {
    title_.set(0, std::format(L"{} tracks selected", counts.all));
    for (StatusRow& row : rows_) {
        row.count.set(status_icon(row.status), std::format(L"{}", status_count(counts, row.status)));
    }
    counts_ = counts;
    layout();
}

LRESULT SelectionCard::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE: layout(); return 0;
    default:      return Panel::handle_message(message, wparam, lparam);
    }
}

SelectionCard::StatusRow SelectionCard::status_row(StatusTab status) {
    return {
        .status = status,
        .key = framework::Label(hwnd(), status_word(status), {.color = framework::TextColor::Gray}),
        .count = IconText(hwnd(), CountStyle),
    };
}

int SelectionCard::arrange(int width, framework::DeferredPositions* positions) const {
    framework::Dpi dpi = framework::Dpi::of(hwnd());
    CardLayout layout(dpi, width, line_height_, positions);
    layout.title(title_.hwnd(), title_.height());
    layout.section(section_.hwnd());
    for (const StatusRow& row : rows_) {
        layout.row_if(status_count(counts_, row.status) > 0, row.key.hwnd(), row.count.hwnd());
    }
    if (counts_.all > 0 && counts_.removed == counts_.all) {
        layout.block(all_removed_.hwnd(), dpi.scale(AllRemovedGapDips), all_removed_.height_for(width));
    } else {
        layout.hide(all_removed_.hwnd());
    }
    return layout.height();
}

void SelectionCard::layout() {
    RECT client;
    platform::win32::check(GetClientRect(hwnd(), &client), "GetClientRect");
    framework::DeferredPositions positions;
    arrange(client.right, &positions);
    positions.apply();
}

}
