# CLAUDE.md

This file provides guidance when working with code in this repository.

WestieLib is a private Win32 desktop app: C++26, MinGW GCC, one static exe. It keeps YouTube Music and Spotify
playlists in sync with a folder of tagged audio files for Mixxx.

## Read next

| File | When |
|------|------|
| `docs/build.md` | Build commands, CMake setup, resources (icon, manifest, version) |
| `docs/ui-framework.md` | Before writing UI code: window lifecycle, exception handling, framework classes |
| `docs/main-window.md` | MainWindow: startup and shutdown, layout code, saved window state |
| `docs/testing.md` | Checking a change: `tests/ui/ui-checks.ps1`, driving the app from PowerShell |
| `plan/arch.md` | Architecture, data model and core API |
| `plan/ui.md` | UI spec. `plan/ui-mockup.html` is the visual reference; it is large, so grep it |

## Build and check

```powershell
$env:PATH = "C:\msys64\mingw64\bin;$env:PATH"
$cmake = "C:\Users\Levi van Rheenen\AppData\Local\Programs\CLion\bin\cmake\win\x64\bin\cmake.exe"
& $cmake -S "D:\C++\WestieLib" -B $build -G Ninja "-DCMAKE_MAKE_PROGRAM=C:\Users\Levi van Rheenen\AppData\Local\Programs\CLion\bin\ninja\win\x64\ninja.exe"
& $cmake --build $build
powershell -NoProfile -ExecutionPolicy Bypass -File tests/ui/ui-checks.ps1 -Exe $build\WestieLib.exe -Screenshot <png>
```

Use the session scratchpad as `$build`; `cmake-build-debug/` belongs to CLion. The build must have zero warnings, and
`ui-checks.ps1` must report 0 failures.

## Workflow

- UI changes start in the spec: update `plan/ui.md` (and the mockup if something visible changes), then write C++.
  The user sketches layouts in ASCII; follow them, and say where you depart from them.
- The user works in phases and reviews each one, so stop after a phase.
- It's a private app: real brand logos are fine, and brand-guideline caveats aren't needed.

## Code rules

- Naming: PascalCase for types, `constexpr` constants, non-type template parameters, enums and their values.
  UPPER_SNAKE_CASE for macros. snake_case for everything else. Private members end in `_`. Files are named after
  their class (`MainWindow.h`); folders are lowercase.
- One class per `.h`/`.cpp` pair; a small data struct may sit with the function that uses it. `#pragma once`.
- No `using namespace` and no using-declarations. Qualify names, as in `platform::win32::check(...)`.
- No closing-namespace comments.
- `const` only on references and where it changes the meaning (const member functions, pointers to const), never on
  locals or by-value parameters. Use `constexpr` constants instead of magic numbers.
- Hardly any comments: none on classes or functions. Write one only to give a reason the code can't show.
- Formatting follows `.clang-format`, but by hand: the clang-format on PATH (`C:\LLVM\bin`, version 10) is too old to
  read that file.
  - 4 spaces, 160 columns, braced lists written `{a, b}`;
  - case labels flush with `switch`; short cases on one line, with their statements aligned;
  - `X() : a(1) {}` on one line when it fits, otherwise the initializers on the next lines, indented 8;
  - long calls break after `(`, with the arguments indented 8 and `)` on its own line.
- Include order: the file's own header; std headers; a `<windows.h>` / `<windowsx.h>` block; project headers from
  `src/`, sorted case-sensitively (`"ResourceIds.h"` before `"platform/..."`). Separate the blocks with blank lines.
- Win32: use the W APIs. Check results with `platform::win32::check` / `check_status`, wrap every handle in RAII
  (`UniqueHandle`), and keep sizes in DIPs, scaled with `Dpi` where they're used. `platform/` never includes `ui/`.
- Add every new `.cpp` to `add_executable` in `CMakeLists.txt`.

## Gotchas

- Messages that arrive during a `Window`'s base construction get default handling. Set up in the derived
  constructor, not in `WM_CREATE`.
- Don't catch exceptions in window handlers to keep them out of Win32; `Window::window_proc` already parks them.
- `DeferredPositions` uses `SWP_NOCOPYBITS` and `RDW_UPDATENOW`; removing them brings back half-painted splitter drags.
- Because of that, every drag step repaints the panels it touches. A window that paints its own content uses
  `BufferedPaint` and doesn't erase its background first, or its text blinks while a splitter moves.
- Give struct fields default member initializers. Otherwise GCC warns whenever a designated initializer leaves one out.
- The app saves `HKCU\Software\WestieLib\Window` on every close, and `%LOCALAPPDATA%\WestieLib\settings.json` when the
  library folder changes. Anything that starts it must put the user's values and file back afterwards; see
  `docs/testing.md`.
