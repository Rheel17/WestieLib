#include "ui/dialogs/ChooseVideoDialog.h"

#include <array>
#include <format>
#include <string_view>
#include <utility>

#include <commctrl.h>

#include "ResourceIds.h"
#include "core/matching/VideoLink.h"
#include "core/sources/SourcePage.h"
#include "platform/win32/ShellOpen.h"
#include "platform/win32/Utf16.h"
#include "platform/win32/Win32Error.h"
#include "ui/dialogs/LinkLinePainter.h"
#include "ui/framework/EditKeys.h"

namespace westie::ui {

namespace {

constexpr std::wstring_view Spaces = L" \t\r\n";
// Windows lays the controls out again from the template after WM_DPICHANGED, so the anchors place them after that
constexpr UINT RelayoutMessage = WM_APP;

constexpr std::array Anchors = {
    framework::ControlAnchor{.id = IDC_VIDEO_SOURCE, .horizontal = framework::Anchor::Stretch},
    framework::ControlAnchor{.id = IDC_PLAY_SOURCE, .horizontal = framework::Anchor::End},
    framework::ControlAnchor{.id = IDC_VIDEO_CURRENT, .horizontal = framework::Anchor::Stretch},
    framework::ControlAnchor{.id = IDC_VIDEO_QUERY, .horizontal = framework::Anchor::Stretch},
    framework::ControlAnchor{.id = IDC_VIDEO_SEARCH, .horizontal = framework::Anchor::End},
    framework::ControlAnchor{.id = IDC_VIDEO_LIST, .horizontal = framework::Anchor::Stretch, .vertical = framework::Anchor::Stretch},
    framework::ControlAnchor{.id = IDC_VIDEO_LINK_LABEL, .vertical = framework::Anchor::End},
    framework::ControlAnchor{.id = IDC_VIDEO_LINK, .horizontal = framework::Anchor::Stretch, .vertical = framework::Anchor::End},
    framework::ControlAnchor{.id = IDC_PLAY_LINK, .horizontal = framework::Anchor::End, .vertical = framework::Anchor::End},
    framework::ControlAnchor{.id = IDC_VIDEO_LINK_KIND, .horizontal = framework::Anchor::Stretch, .vertical = framework::Anchor::End},
    framework::ControlAnchor{.id = IDOK, .horizontal = framework::Anchor::End, .vertical = framework::Anchor::End},
    framework::ControlAnchor{.id = IDCANCEL, .horizontal = framework::Anchor::End, .vertical = framework::Anchor::End},
    framework::ControlAnchor{.id = IDC_VIDEO_GRIP, .horizontal = framework::Anchor::End, .vertical = framework::Anchor::End},
};

bool is_blank(std::wstring_view text) {
    return text.find_first_not_of(Spaces) == std::wstring_view::npos;
}

std::wstring video_page(const std::string& video_id) {
    return platform::win32::to_utf16(std::format("https://music.youtube.com/watch?v={}", video_id));
}

}

ChooseVideoDialog::ChooseVideoDialog(VideoChoice choice, core::VideoSearch& search, LibraryVideoTitle library_title, framework::Size window_dips) :
        ModalDialog(IDD_CHOOSE_VIDEO),
        choice_(std::move(choice)),
        search_(search),
        library_title_(std::move(library_title)),
        window_dips_(window_dips) {}

std::optional<std::string> ChooseVideoDialog::choose(HWND owner) {
    INT_PTR result = show_modal(owner);
    search_.stop();
    return result == IDOK ? chosen_ : std::nullopt;
}

void ChooseVideoDialog::show_results(const core::VideoSearchResults& results) {
    // An answer to a search that a newer one replaced is left out
    if (!hwnd() || results.request != request_) {
        return;
    }
    candidates_ = results.candidates;
    list_->show(candidates_, choice_.duration);
    list_->set_empty_text(
            results.error.empty() ? L"No results. Try fewer words, or paste a link below."
                                  : std::format(L"Couldn't search YouTube Music: {}", platform::win32::to_utf16(results.error))
    );
    show_buttons();
}

framework::Size ChooseVideoDialog::window_dips() const noexcept {
    return window_dips_;
}

INT_PTR ChooseVideoDialog::handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_INITDIALOG: on_init_dialog(); return TRUE;
    case WM_COMMAND:    on_command(wparam); return TRUE;
    case WM_NOTIFY:     return on_notify(*reinterpret_cast<NMHDR*>(lparam));
    case WM_DRAWITEM:   on_draw_item(*reinterpret_cast<const DRAWITEMSTRUCT*>(lparam)); return TRUE;
    case WM_GETMINMAXINFO:
        // It comes before WM_INITDIALOG too
        if (sizing_) {
            sizing_->limit(*reinterpret_cast<MINMAXINFO*>(lparam));
        }
        return TRUE;
    case WM_SIZE:
        if (sizing_) {
            sizing_->place();
        }
        return TRUE;
    case WM_DPICHANGED: PostMessageW(hwnd(), RelayoutMessage, 0, 0); return FALSE;
    case RelayoutMessage: on_dpi_changed(); return TRUE;
    case WM_DESTROY:
        if (sizing_) {
            window_dips_ = sizing_->window_dips();
        }
        return FALSE;
    default: return ModalDialog::handle_message(message, wparam, lparam);
    }
}

void ChooseVideoDialog::on_init_dialog() {
    // The line paints itself, so its window text is only for screen readers and the UI checks
    std::wstring heading = choice_.heading.accessible_text();
    platform::win32::check(SetDlgItemTextW(hwnd(), IDC_VIDEO_SOURCE, heading.c_str()), "SetDlgItemTextW");
    platform::win32::check(SetDlgItemTextW(hwnd(), IDC_VIDEO_CURRENT, choice_.current.c_str()), "SetDlgItemTextW");
    platform::win32::check(SetDlgItemTextW(hwnd(), IDC_VIDEO_QUERY, choice_.query.c_str()), "SetDlgItemTextW");
    framework::add_edit_keys(platform::win32::check(GetDlgItem(hwnd(), IDC_VIDEO_QUERY), "GetDlgItem"));
    framework::add_edit_keys(platform::win32::check(GetDlgItem(hwnd(), IDC_VIDEO_LINK), "GetDlgItem"));
    list_.emplace(platform::win32::check(GetDlgItem(hwnd(), IDC_VIDEO_LIST), "GetDlgItem"), [this](std::size_t row) {
        if (row < candidates_.size()) {
            play_video(candidates_[row].video_id);
        }
    });
    source_play_.emplace(platform::win32::check(GetDlgItem(hwnd(), IDC_PLAY_SOURCE), "GetDlgItem"), L"Play on Spotify");
    link_play_.emplace(platform::win32::check(GetDlgItem(hwnd(), IDC_PLAY_LINK), "GetDlgItem"), L"Play on YouTube Music");
    sizing_.emplace(hwnd(), Anchors);
    sizing_->resize(window_dips_);
    center_on_owner();
    search(std::nullopt);
    check_link();
}

void ChooseVideoDialog::on_command(WPARAM wparam) {
    switch (LOWORD(wparam)) {
    case IDOK:
        // Enter in the search field searches
        if (GetFocus() == GetDlgItem(hwnd(), IDC_VIDEO_QUERY)) {
            search_typed();
        } else {
            accept();
        }
        return;
    case IDCANCEL:         end(IDCANCEL); return;
    case IDC_VIDEO_SEARCH: search_typed(); return;
    case IDC_PLAY_SOURCE:  play_source(); return;
    case IDC_PLAY_LINK:
        if (link_) {
            play_video(*link_);
        }
        return;
    case IDC_VIDEO_LINK:
        if (HIWORD(wparam) == EN_CHANGE) {
            check_link();
        }
        return;
    default: return;
    }
}

INT_PTR ChooseVideoDialog::on_notify(NMHDR& header) {
    if (header.idFrom != IDC_VIDEO_LIST) {
        return FALSE;
    }
    switch (header.code) {
    case LVN_ITEMCHANGED: show_buttons(); return TRUE;
    case NM_CUSTOMDRAW:   return message_result(list_->on_custom_draw(reinterpret_cast<NMLVCUSTOMDRAW&>(header)));
    case LVN_GETEMPTYMARKUP:
        list_->on_empty_markup(reinterpret_cast<NMLVEMPTYMARKUP&>(header));
        return message_result(TRUE);
    case NM_DBLCLK:
        // The row, even while the link names another video
        if (std::optional<std::size_t> row = list_->selected(); row && *row < candidates_.size()) {
            chosen_ = candidates_[*row].video_id;
            end(IDOK);
        }
        return TRUE;
    default: return FALSE;
    }
}

void ChooseVideoDialog::on_draw_item(const DRAWITEMSTRUCT& item) const {
    if (item.CtlID == IDC_VIDEO_SOURCE) {
        paint_link_line(item, choice_.heading, 0.0);
    } else if (item.CtlID == IDC_VIDEO_LINK_KIND) {
        paint_link_line(item, link_line_, 0.0);
    }
}

void ChooseVideoDialog::on_dpi_changed() {
    sizing_->place();
    source_play_->apply_dpi();
    link_play_->apply_dpi();
}

void ChooseVideoDialog::search(std::optional<std::string> query) {
    request_ = search_.search(choice_.source, std::move(query));
    candidates_.clear();
    list_->show(candidates_, choice_.duration);
    list_->set_empty_text(L"Searching YouTube Music…");
    show_buttons();
}

void ChooseVideoDialog::search_typed() {
    std::wstring query = item_text(IDC_VIDEO_QUERY);
    if (is_blank(query)) {
        MessageBeep(MB_OK);
        return;
    }
    search(platform::win32::to_utf8(query));
}

void ChooseVideoDialog::check_link() {
    std::wstring text = item_text(IDC_VIDEO_LINK);
    link_ = core::parse_video_link(platform::win32::to_utf8(text));
    if (link_) {
        std::optional<std::string> title = library_title_(*link_);
        link_line_ = {
            .icon = IDI_YTM,
            .text = std::format(L"YouTube video {}", platform::win32::to_utf16(*link_)),
            .source = title ? platform::win32::to_utf16(*title) : L"",
        };
    } else if (is_blank(text)) {
        link_line_ = {};
    } else {
        link_line_ = {.text = L"Not a YouTube link", .gray = true};
    }

    std::wstring accessible = link_line_.accessible_text();
    HWND line = GetDlgItem(hwnd(), IDC_VIDEO_LINK_KIND);
    platform::win32::check(SetWindowTextW(line, accessible.c_str()), "SetWindowTextW");
    platform::win32::check(InvalidateRect(line, nullptr, FALSE), "InvalidateRect");
    show_buttons();
}

void ChooseVideoDialog::show_buttons() {
    EnableWindow(GetDlgItem(hwnd(), IDOK), chosen_video().has_value());
    if (link_play_) {
        link_play_->set_enabled(link_.has_value());
    }
}

std::optional<std::string> ChooseVideoDialog::chosen_video() const {
    // A pasted link wins over the list
    if (link_) {
        return link_;
    }
    if (std::optional<std::size_t> row = list_ ? list_->selected() : std::nullopt; row && *row < candidates_.size()) {
        return candidates_[*row].video_id;
    }
    return std::nullopt;
}

void ChooseVideoDialog::play_source() const {
    if (std::optional<std::string> page = core::source_page_url(choice_.source)) {
        platform::win32::open_with_shell(hwnd(), platform::win32::to_utf16(*page));
    }
}

void ChooseVideoDialog::play_video(const std::string& video_id) const {
    platform::win32::open_with_shell(hwnd(), video_page(video_id));
}

void ChooseVideoDialog::accept() {
    // IDOK also comes from Enter, while the button is disabled
    chosen_ = chosen_video();
    if (chosen_) {
        end(IDOK);
    }
}

}
