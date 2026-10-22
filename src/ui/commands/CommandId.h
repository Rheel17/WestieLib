#pragma once

#include <windows.h>

namespace westie::ui {

enum class CommandId : WORD {
    AddPlaylist = 100,
    RemovePlaylist,
    OpenLibraryFolder,
    Exit,
    SyncAll,
    SyncSelectedPlaylist,
    CancelSync,
    ConfirmMatch,
    ChooseVideo,
    EditTrack,
    Retry,
    Keep,
    MoveToRecycleBin,
    ShowInFolder,
    OpenOnYouTube,
    OpenSourcePage,
    ToggleLog,
    Columns,
    Settings,
    CheckForUpdates,
    RebuildLibrary,
    RelocateLibrary,
    OpenLogFolder,
    About,
    // Not in the menu bar: the playlist card's link and the playlist context menu
    OpenPlaylistPage,
    CopyPlaylistLink,
};

}
