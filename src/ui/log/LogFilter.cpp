#include "ui/log/LogFilter.h"

namespace westie::ui {

bool log_filter_shows(const LogFilter& filter, core::LogLevel level, const core::LogSubject& subject) {
    if (level < filter.view.least) {
        return false;
    }
    const LogSelection& selection = filter.selection;
    switch (filter.view.scope) {
    case LogScope::All: return true;
    case LogScope::Playlist:
        // All tracks is the whole library
        if (!selection.playlist) {
            return true;
        }
        return subject.playlist == *selection.playlist || (!subject.track.empty() && selection.playlist_tracks.contains(subject.track));
    case LogScope::Track: return !subject.track.empty() && selection.tracks.contains(subject.track);
    }
    return true;
}

bool log_filters_differ(const LogFilter& a, const LogFilter& b) {
    if (a.view != b.view) {
        return true;
    }
    switch (a.view.scope) {
    case LogScope::All:      return false;
    case LogScope::Playlist: return a.selection.playlist != b.selection.playlist || a.selection.playlist_tracks != b.selection.playlist_tracks;
    case LogScope::Track:    return a.selection.tracks != b.selection.tracks;
    }
    return true;
}

}
