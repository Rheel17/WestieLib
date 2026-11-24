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

# Tags in the downloaded files; static, without zlib, which only compressed ID3v2 frames need. Its CMake files are
# older than option() honoring variables, unless the policy says so.
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(BUILD_SHARED_LIBS OFF)
set(WITH_ZLIB OFF)
set(BUILD_TESTING OFF)
set(BUILD_EXAMPLES OFF)
set(BUILD_BINDINGS OFF)
FetchContent_Declare(taglib
	GIT_REPOSITORY https://github.com/taglib/taglib.git
	GIT_TAG v2.1.1
	GIT_SHALLOW TRUE
	GIT_SUBMODULES 3rdparty/utfcpp
	SYSTEM)
FetchContent_MakeAvailable(taglib)

# The Matcher's rules, the search's timeout, the log's texts and what they use, which the unit checks build as well
set(MATCHING_SOURCES
	src/core/log/LogText.cpp
	src/core/matching/Credits.cpp
	src/core/matching/MatchEvidence.cpp
	src/core/matching/MatchJudgement.cpp
	src/core/matching/MatchRank.cpp
	src/core/matching/MatchText.cpp
	src/core/matching/MatchWords.cpp
	src/core/matching/NameMatch.cpp
	src/core/matching/SearchLogText.cpp
	src/core/matching/SearchTimeout.cpp
	src/core/matching/SongFacts.cpp
	src/core/matching/TitleParts.cpp
	src/platform/win32/Utf16.cpp
	src/platform/win32/Win32Error.cpp
	src/platform/win32/Win32TextNormalizer.cpp
)
set(WINDOWS_DEFINITIONS UNICODE _UNICODE STRICT WIN32_LEAN_AND_MEAN NOMINMAX WINVER=0x0A00 _WIN32_WINNT=0x0A00)
# libstdc++ checks preconditions such as std::clamp's bounds only in unoptimized builds unless asked, and the UI checks
# should catch a broken one whatever the build type
set(LIBSTDCXX_DEFINITIONS _GLIBCXX_ASSERTIONS)

add_executable(WestieLib WIN32
	src/main.cpp
	${MATCHING_SOURCES}
	src/core/github/GitHubReleases.cpp
	src/core/json/IsoTime.cpp
	src/core/json/Utf8Path.cpp
	src/core/log/Log.cpp
	src/core/log/LogSubject.cpp
	src/core/matching/CandidateSearch.cpp
	src/core/matching/ConfirmMatch.cpp
	src/core/matching/LibraryMatches.cpp
	src/core/matching/MatchCandidates.cpp
	src/core/matching/MatchQueries.cpp
	src/core/matching/Matcher.cpp
	src/core/matching/RefusedSearch.cpp
	src/core/matching/YouTubeMusicLink.cpp
	src/core/matching/YouTubeMusicSearch.cpp
	src/core/matching/YtmOverride.cpp
	src/core/playlists/AddPlaylist.cpp
	src/core/playlists/PlaylistLink.cpp
	src/core/playlists/PlaylistLookup.cpp
	src/core/playlists/RemovePlaylist.cpp
	src/core/settings/SettingsStore.cpp
	src/core/sources/LinkUrl.cpp
	src/core/sources/SourcePage.cpp
	src/core/sources/SourceType.cpp
	src/core/sources/SpotifyEmbedPage.cpp
	src/core/sources/SpotifyPlaylist.cpp
	src/core/sources/YouTubeMusicPlaylist.cpp
	src/core/sources/YouTubeMusicPlaylistName.cpp
	src/core/sources/YtDlpError.cpp
	src/core/state/EffectiveMetadata.cpp
	src/core/state/ListedSources.cpp
	src/core/state/MatchReview.cpp
	src/core/state/MergeTracks.cpp
	src/core/state/PruneTracks.cpp
	src/core/state/SplitSource.cpp
	src/core/state/StateStore.cpp
	src/core/state/TrackYtmIds.cpp
	src/core/sync/ApplyFetch.cpp
	src/core/sync/ApplyMatch.cpp
	src/core/sync/SyncEngine.cpp
	src/core/sync/TracksToMatch.cpp
	src/core/tools/Ffmpeg.cpp
	src/core/tools/ToolFolder.cpp
	src/core/tools/ToolInstaller.cpp
	src/core/tools/ToolManager.cpp
	src/core/tools/ToolPackage.cpp
	src/core/tools/ToolRunner.cpp
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
	src/platform/win32/Win32AppendFile.cpp
	src/platform/win32/Win32FileOps.cpp
	src/platform/win32/Win32ProcessRunner.cpp
	src/platform/win32/Win32TimeZone.cpp
	src/platform/win32/WinHttpClient.cpp
	src/ui/ArtistsText.cpp
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
	src/ui/dialogs/CandidateListView.cpp
	src/ui/dialogs/ChooseMatchDialog.cpp
	src/ui/dialogs/FolderPicker.cpp
	src/ui/dialogs/LinkLine.cpp
	src/ui/dialogs/LinkLinePainter.cpp
	src/ui/dialogs/LinkLookup.cpp
	src/ui/dialogs/MatchChoice.cpp
	src/ui/dialogs/PlayButton.cpp
	src/ui/dialogs/PlaylistLinkCheck.cpp
	src/ui/dialogs/RemovePlaylistDialog.cpp
	src/ui/dialogs/RowPlayButtons.cpp
	src/ui/dialogs/SettingsDialog.cpp
	src/ui/dialogs/ToolListView.cpp
	src/ui/events/CoreEventQueue.cpp
	src/ui/framework/AcceleratorTable.cpp
	src/ui/framework/Animation.cpp
	src/ui/framework/BufferedPaint.cpp
	src/ui/framework/Button.cpp
	src/ui/framework/CloseButton.cpp
	src/ui/framework/ComboBox.cpp
	src/ui/framework/CommonControls.cpp
	src/ui/framework/Control.cpp
	src/ui/framework/DeferredPositions.cpp
	src/ui/framework/DialogSizing.cpp
	src/ui/framework/Edit.cpp
	src/ui/framework/EditHistory.cpp
	src/ui/framework/EditKeys.cpp
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
	src/ui/framework/WordBreaks.cpp
	src/ui/infobar/FetchFailureNotice.cpp
	src/ui/infobar/Infobar.cpp
	src/ui/inspector/AudioRows.cpp
	src/ui/inspector/CardLayout.cpp
	src/ui/inspector/ConfidenceValue.cpp
	src/ui/inspector/CoverArt.cpp
	src/ui/inspector/DoubtList.cpp
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
	src/ui/log/LogFilter.cpp
	src/ui/log/LogLines.cpp
	src/ui/log/LogPanel.cpp
	src/ui/log/LogTable.cpp
	src/ui/log/LogToolbar.cpp
	src/ui/log/SelectedForLog.cpp
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
	src/ui/tracks/RowStatusText.cpp
	src/ui/tracks/StatusFilter.cpp
	src/ui/tracks/StatusTabText.cpp
	src/ui/tracks/TrackEntries.cpp
	src/ui/tracks/TrackPanel.cpp
	src/ui/tracks/TrackReview.cpp
	src/ui/tracks/TrackRows.cpp
	src/ui/tracks/TrackSearch.cpp
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

target_compile_definitions(WestieLib PRIVATE ${WINDOWS_DEFINITIONS} ${LIBSTDCXX_DEFINITIONS})
target_compile_options(WestieLib PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -municode>)
target_link_options(WestieLib PRIVATE -municode -static)
target_link_libraries(WestieLib PRIVATE glaze::glaze miniz tag comctl32 uxtheme gdiplus shell32 ole32 uuid winhttp bcrypt normaliz)

# The unit checks: a console program that tests/ui/ui-checks.ps1 runs as its unit suite
add_executable(WestieLibTests
	tests/unit/main.cpp
	tests/unit/Checks.cpp
	tests/unit/JudgementChecks.cpp
	tests/unit/LogChecks.cpp
	tests/unit/MatchWordsChecks.cpp
	tests/unit/NameChecks.cpp
	tests/unit/RunPhaseChecks.cpp
	tests/unit/SearchTimeoutChecks.cpp
	tests/unit/TitlePartsChecks.cpp
	${MATCHING_SOURCES}
	# The update's phases, as the playlist rows show them
	src/core/tools/ToolPackage.cpp
	src/platform/win32/LocalTime.cpp
	src/ui/RelativeDayText.cpp
	src/ui/playlists/LastSyncLine.cpp
	src/ui/sync/SyncBoard.cpp
	src/ui/sync/WaitingText.cpp
)
target_include_directories(WestieLibTests PRIVATE src)
target_compile_definitions(WestieLibTests PRIVATE ${WINDOWS_DEFINITIONS} ${LIBSTDCXX_DEFINITIONS})
target_compile_options(WestieLibTests PRIVATE -Wall -Wextra)
target_link_options(WestieLibTests PRIVATE -static)
target_link_libraries(WestieLibTests PRIVATE normaliz)
