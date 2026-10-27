cmake_minimum_required(VERSION 4.3)
project(WestieLib VERSION 0.1.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(src/Version.h.in "${GENERATED_DIR}/Version.h" @ONLY)

# JSON for settings.json; SYSTEM keeps its headers out of the warnings
include(FetchContent)
FetchContent_Declare(glaze
	GIT_REPOSITORY https://github.com/stephenberry/glaze.git
	GIT_TAG v9.0.0
	GIT_SHALLOW TRUE
	SYSTEM)
FetchContent_MakeAvailable(glaze)

# Zip extraction for the tools; SYSTEM keeps its headers out of the warnings
FetchContent_Declare(miniz
	GIT_REPOSITORY https://github.com/richgel999/miniz.git
	GIT_TAG 3.1.2
	GIT_SHALLOW TRUE
	SYSTEM)
FetchContent_MakeAvailable(miniz)

add_executable(WestieLib WIN32
	src/main.cpp
	src/core/github/GitHubReleases.cpp
	src/core/json/IsoTime.cpp
	src/core/json/Utf8Path.cpp
	src/core/matching/MatchScore.cpp
	src/core/matching/MatchText.cpp
	src/core/matching/Matcher.cpp
	src/core/matching/YouTubeMusicSearch.cpp
	src/core/playlists/AddPlaylist.cpp
	src/core/playlists/PlaylistLink.cpp
	src/core/playlists/PlaylistLookup.cpp
	src/core/playlists/RemovePlaylist.cpp
	src/core/settings/SettingsStore.cpp
	src/core/sources/SourcePage.cpp
	src/core/sources/SourceType.cpp
	src/core/sources/SpotifyEmbedPage.cpp
	src/core/sources/SpotifyPlaylist.cpp
	src/core/sources/YouTubeMusicPlaylist.cpp
	src/core/sources/YouTubeMusicPlaylistName.cpp
	src/core/sources/YtDlpError.cpp
	src/core/state/EffectiveMetadata.cpp
	src/core/state/MergeTracks.cpp
	src/core/state/PruneTracks.cpp
	src/core/state/StateStore.cpp
	src/core/state/TrackVideos.cpp
	src/core/sync/ApplyFetch.cpp
	src/core/sync/ApplyMatch.cpp
	src/core/sync/SyncEngine.cpp
	src/core/sync/TracksToMatch.cpp
	src/core/tools/ToolFolder.cpp
	src/core/tools/ToolInstaller.cpp
	src/core/tools/ToolManager.cpp
	src/core/tools/ToolPackage.cpp
	src/core/tools/ToolSource.cpp
	src/core/tools/ToolsStore.cpp
	src/core/tools/YtDlp.cpp
	src/platform/NearestExistingFolder.cpp
	src/platform/win32/BCryptHasher.cpp
	src/platform/win32/Clipboard.cpp
	src/platform/win32/ClipboardScope.cpp
	src/platform/win32/ComApartment.cpp
	src/platform/win32/CommandLine.cpp
	src/platform/win32/Environment.cpp
	src/platform/win32/HandleInheritanceList.cpp
	src/platform/win32/KnownFolder.cpp
	src/platform/win32/LocalTime.cpp
	src/platform/win32/MinizArchiveExtractor.cpp
	src/platform/win32/PathProbe.cpp
	src/platform/win32/Pipe.cpp
	src/platform/win32/RegistryKey.cpp
	src/platform/win32/ShellOpen.cpp
	src/platform/win32/Utf16.cpp
	src/platform/win32/Win32Error.cpp
	src/platform/win32/Win32FileOps.cpp
	src/platform/win32/Win32ProcessRunner.cpp
	src/platform/win32/Win32TextNormalizer.cpp
	src/platform/win32/WinHttpClient.cpp
	src/ui/DurationText.cpp
	src/ui/FatalError.cpp
	src/ui/PlaylistTypeText.cpp
	src/ui/RelativeDayText.cpp
	src/ui/RowLook.cpp
	src/ui/SourceLogo.cpp
	src/ui/SyncBar.cpp
	src/ui/TrackCountText.cpp
	src/ui/TrackCounts.cpp
	src/ui/commands/CommandMenu.cpp
	src/ui/commands/CommandState.cpp
	src/ui/dialogs/AboutDialog.cpp
	src/ui/dialogs/AddPlaylistDialog.cpp
	src/ui/dialogs/FolderPicker.cpp
	src/ui/dialogs/LinkLine.cpp
	src/ui/dialogs/LinkLinePainter.cpp
	src/ui/dialogs/LinkLookup.cpp
	src/ui/dialogs/PlaylistLinkCheck.cpp
	src/ui/dialogs/RemovePlaylistDialog.cpp
	src/ui/dialogs/SettingsDialog.cpp
	src/ui/dialogs/ToolListView.cpp
	src/ui/events/CoreEventQueue.cpp
	src/ui/framework/AcceleratorTable.cpp
	src/ui/framework/Animation.cpp
	src/ui/framework/BufferedPaint.cpp
	src/ui/framework/Button.cpp
	src/ui/framework/ComboBox.cpp
	src/ui/framework/CommonControls.cpp
	src/ui/framework/Control.cpp
	src/ui/framework/DeferredPositions.cpp
	src/ui/framework/Edit.cpp
	src/ui/framework/FocusMemory.cpp
	src/ui/framework/FontSet.cpp
	src/ui/framework/GdiPlusSession.cpp
	src/ui/framework/IconImageList.cpp
	src/ui/framework/IconResource.cpp
	src/ui/framework/IconView.cpp
	src/ui/framework/ItemTracker.cpp
	src/ui/framework/Label.cpp
	src/ui/framework/LinkLabel.cpp
	src/ui/framework/ListView.cpp
	src/ui/framework/MenuBarPainter.cpp
	src/ui/framework/MessageFont.cpp
	src/ui/framework/MessageLoop.cpp
	src/ui/framework/ModalDialog.cpp
	src/ui/framework/Panel.cpp
	src/ui/framework/PendingException.cpp
	src/ui/framework/PopupMenu.cpp
	src/ui/framework/ProgressBar.cpp
	src/ui/framework/Spinner.cpp
	src/ui/framework/Splitter.cpp
	src/ui/framework/StatusBar.cpp
	src/ui/framework/TabStrip.cpp
	src/ui/framework/TextColor.cpp
	src/ui/framework/Toolbar.cpp
	src/ui/framework/Tooltip.cpp
	src/ui/framework/VerticalScroll.cpp
	src/ui/framework/Window.cpp
	src/ui/framework/WindowClass.cpp
	src/ui/framework/WindowIcon.cpp
	src/ui/infobar/FetchFailureNotice.cpp
	src/ui/infobar/Infobar.cpp
	src/ui/inspector/AudioRows.cpp
	src/ui/inspector/CardLayout.cpp
	src/ui/inspector/ConfidenceValue.cpp
	src/ui/inspector/CoverArt.cpp
	src/ui/inspector/IconText.cpp
	src/ui/inspector/InspectorPanel.cpp
	src/ui/inspector/LengthValue.cpp
	src/ui/inspector/LibraryCard.cpp
	src/ui/inspector/OutputBox.cpp
	src/ui/inspector/PlaylistCard.cpp
	src/ui/inspector/SectionTitle.cpp
	src/ui/inspector/SelectionCard.cpp
	src/ui/inspector/SourceLink.cpp
	src/ui/inspector/SyncedValue.cpp
	src/ui/inspector/TrackCard.cpp
	src/ui/inspector/TrackDetails.cpp
	src/ui/inspector/TrackStatusText.cpp
	src/ui/log/LogPanel.cpp
	src/ui/main/MainAccelerators.cpp
	src/ui/main/MainLayout.cpp
	src/ui/main/MainMenu.cpp
	src/ui/main/MainWindow.cpp
	src/ui/main/WindowPlacement.cpp
	src/ui/main/WindowState.cpp
	src/ui/playlists/AllTracksRow.cpp
	src/ui/playlists/LastSyncLine.cpp
	src/ui/playlists/PlaylistList.cpp
	src/ui/playlists/PlaylistMenu.cpp
	src/ui/playlists/PlaylistPanel.cpp
	src/ui/playlists/PlaylistRowPainter.cpp
	src/ui/playlists/PlaylistRows.cpp
	src/ui/statusbar/MainStatusBar.cpp
	src/ui/sync/SyncBoard.cpp
	src/ui/sync/WaitingText.cpp
	src/ui/toolbar/MainToolbar.cpp
	src/ui/tools/ToolBoard.cpp
	src/ui/tools/ToolMark.cpp
	src/ui/tools/ToolStatusText.cpp
	src/ui/tracks/FilterBar.cpp
	src/ui/tracks/ResultLineText.cpp
	src/ui/tracks/StatusFilter.cpp
	src/ui/tracks/StatusTabText.cpp
	src/ui/tracks/TrackPanel.cpp
	src/ui/tracks/TrackRows.cpp
	src/ui/tracks/TrackSort.cpp
	src/ui/tracks/TrackTable.cpp
	src/ui/welcome/LibraryFolderCheck.cpp
	src/ui/welcome/LibraryFolderField.cpp
	src/ui/welcome/LibraryFolderText.cpp
	src/ui/welcome/WelcomePanel.cpp
	res/WestieLib.rc
)

# windres doesn't track the files the .rc pulls in
file(GLOB GLYPH_ICONS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/res/glyphs/*.ico")
set_source_files_properties(res/WestieLib.rc PROPERTIES OBJECT_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/res/WestieLib.ico;${CMAKE_CURRENT_SOURCE_DIR}/res/WestieLib.manifest;${GLYPH_ICONS}")

# windres also searches these directories for the files named in the .rc
target_include_directories(WestieLib PRIVATE src res "${GENERATED_DIR}")

target_compile_definitions(WestieLib PRIVATE
	UNICODE _UNICODE STRICT WIN32_LEAN_AND_MEAN NOMINMAX WINVER=0x0A00 _WIN32_WINNT=0x0A00)
target_compile_options(WestieLib PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -municode>)
target_link_options(WestieLib PRIVATE -municode -static)
target_link_libraries(WestieLib PRIVATE glaze::glaze miniz comctl32 uxtheme gdiplus shell32 ole32 uuid winhttp bcrypt normaliz)
