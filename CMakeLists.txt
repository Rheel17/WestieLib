cmake_minimum_required(VERSION 4.3)
project(WestieLib VERSION 0.1.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(src/Version.h.in "${GENERATED_DIR}/Version.h" @ONLY)

add_executable(WestieLib WIN32
	src/main.cpp
	src/platform/win32/KnownFolder.cpp
	src/platform/win32/RegistryKey.cpp
	src/platform/win32/Utf16.cpp
	src/platform/win32/Win32Error.cpp
	src/ui/FatalError.cpp
	src/ui/TrackCountText.cpp
	src/ui/commands/CommandMenu.cpp
	src/ui/commands/CommandState.cpp
	src/ui/dialogs/AboutDialog.cpp
	src/ui/framework/AcceleratorTable.cpp
	src/ui/framework/BufferedPaint.cpp
	src/ui/framework/Button.cpp
	src/ui/framework/CommonControls.cpp
	src/ui/framework/Control.cpp
	src/ui/framework/DeferredPositions.cpp
	src/ui/framework/FontSet.cpp
	src/ui/framework/IconImageList.cpp
	src/ui/framework/IconResource.cpp
	src/ui/framework/Label.cpp
	src/ui/framework/MenuBarPainter.cpp
	src/ui/framework/MessageFont.cpp
	src/ui/framework/MessageLoop.cpp
	src/ui/framework/Panel.cpp
	src/ui/framework/PendingException.cpp
	src/ui/framework/PopupMenu.cpp
	src/ui/framework/Splitter.cpp
	src/ui/framework/StatusBar.cpp
	src/ui/framework/Toolbar.cpp
	src/ui/framework/Window.cpp
	src/ui/framework/WindowClass.cpp
	src/ui/framework/WindowIcon.cpp
	src/ui/inspector/InspectorPanel.cpp
	src/ui/log/LogPanel.cpp
	src/ui/main/MainAccelerators.cpp
	src/ui/main/MainLayout.cpp
	src/ui/main/MainMenu.cpp
	src/ui/main/MainWindow.cpp
	src/ui/main/WindowPlacement.cpp
	src/ui/main/WindowState.cpp
	src/ui/playlists/AllTracksRow.cpp
	src/ui/playlists/PlaylistPanel.cpp
	src/ui/statusbar/MainStatusBar.cpp
	src/ui/toolbar/MainToolbar.cpp
	src/ui/tracks/TrackPanel.cpp
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
target_link_libraries(WestieLib PRIVATE comctl32 uxtheme shell32 ole32 uuid)
