#pragma once

#include <windows.h>

namespace westie::ui {

enum class CommandId : WORD {
    AddPlaylist = 100,
    RemovePlaylist,
    OpenLibraryFolder,
    Exit,
    UpdateAll,
    UpdateSelectedPlaylist,
    CancelUpdate,
    ConfirmMatch,
    ChooseMatch,
    EditTrack,
    Retry,
    Keep,
    DeleteFiles,
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
    // Not in the menu bar: the playlist card's link, the playlist context menu and Ctrl+F
    OpenPlaylistPage,
    CopyPlaylistLink,
    FocusSearch,
};

}
