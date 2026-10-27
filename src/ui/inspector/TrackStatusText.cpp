#include "ui/inspector/TrackStatusText.h"

#include "ResourceIds.h"

namespace westie::ui {

int status_icon(StatusTab status) {
    switch (status) {
    case StatusTab::Synced:      return IDI_SYNCED;
    case StatusTab::NeedsReview: return IDI_WARN;
    case StatusTab::Failed:      return IDI_FAILED;
    case StatusTab::Removed:     return IDI_REMOVED;
    case StatusTab::Kept:        return IDI_KEPT;
    default:                     return IDI_WAITING;
    }
}

const wchar_t* status_explanation(StatusTab status, core::TrackFailure failure) {
    // Until Choose video exists, searching again is all there is to say
    if (status == StatusTab::Failed && failure == core::TrackFailure::NoMatch) {
        return L"No usable video was found. It's searched again on the next sync.";
    }
    switch (status) {
    case StatusTab::Synced:  return L"The file is in the library and tagged.";
    case StatusTab::Failed:  return L"The last attempt failed. It's retried on the next sync.";
    case StatusTab::Removed: return L"No longer in any playlist. The file is still in the library.";
    case StatusTab::Kept:    return L"Kept after it left every playlist.";
    // Needs review has a note instead, which comes with matching
    case StatusTab::NeedsReview: return L"";
    default:                     return L"Downloads on the next sync.";
    }
}

const wchar_t* failure_reason(core::TrackFailure failure) {
    switch (failure) {
    case core::TrackFailure::NoMatch: return L"No match on YouTube Music";
    case core::TrackFailure::Search:  return L"Couldn't search YouTube Music";
    default:                          return L"";
    }
}

}
