# WestieLib UI

The UI is a single native Win32 main window with a Windows 10 light look: Segoe UI, system colors, Explorer-style
list views. It has three panels: playlists on the left, tracks in the middle, and an inspector on the right.
Under the toolbar, a window-wide infobar appears when something needs attention. Above the status bar, a
window-wide log pane can be shown.

The static screens in `ui-mockup.html` are the visual reference for this spec.

## Window layout

```
┌ WestieLib ──────────────────────────────────────────────────────────────── ─ □ ✕ ┐
│ File  Sync  Track  View  Tools  Help                                             │ menu bar
│ [+ Add playlist] [⟳ Sync all ▾] [■ Cancel]  [⚙ Settings]                         │ toolbar
│ ⚠ 5 tracks are no longer in any playlist.                  [Review…] [✕]         │ infobar (when needed)
├────────────────────────┬────────────────────────────────────────┬────────────────┤
│ ▤ All tracks           │ [🔍 Search title, artist, album ] Source [All ▾] Clear│ filter bar
│   412 · 398 synced     │ All 412│Synced 398│Waiting 6│Needs review 3│Failed 1… │ status filter
├────────────────────────┤ Showing 214 of 412 · in WCS                     │                │
│ ▶ WCS                  ├────────────────────────────────────────┤ [ cover art ]  │
│   214 tracks           │ # Status  Title    Artists  Album    … │ Title          │
│   ███████████░ 198/214 │ 1 ✓       …        …        Freudian   │ Artists        │
│   14 waiting · 2 failed│ 2 ⚠       …                            │ Album · 2021   │
│   ✓ Synced 14:03 ↻     │ 3 ◔                                    │ ── Status ──   │
│ ● Her playlist         │                                        │ ── Source ──   │
│   98 tracks            │                                        │ ── Audio ──    │
│   ██████░░░░░░  51/98  │                                        │ ── File ──     │
│   Syncing… 6 of 47 ⟳   │                                        │ [actions]      │
│ ● Slow & bluesy        │                                        │                │
│   ⚠ Couldn't fetch ↻   │                                        │                │
│ [+ Add playlist]       │                                        │                │
├────────────────────────┴────────────────────────────────────────┴────────────────┤
│ log pane (hidden by default, View › Log, Ctrl+L)                                 │
├──────────────────────────────────────────────────────────────────────────────────┤
│ Syncing 12 of 40 ▓▓▓▓░░░ │ 412 tracks · D:\Music\WestieLib │ Tools ✓ │ v1.2.0      │ status bar
└──────────────────────────────────────────────────────────────────────────────────┘
```

- The splitters between left|main, main|inspector and panels|log can be dragged. Their positions are saved
  with the window placement.
- The left panel and the inspector are at least as wide as their defaults (250 and 290 px), and the main panel is at
  least 400 px wide. A splitter only trades width with the main panel. When the window gets too narrow for the saved
  widths, the inspector gives way first, then the left panel; the saved widths come back when the window grows.
- The log pane is 168 px tall by default and at least 100 px, and the panels above it keep at least 240 px. Its
  splitter only trades height with the panels. When the window gets too short for the saved height, the log gives
  way; its height comes back when the window grows.
- The infobar row is 32 px tall, right under the toolbar, and takes its height from the panels.
- The window placement (normal bounds and maximized state), the panel widths, the log height and whether the log
  shows are saved under `HKCU\Software\WestieLib\Window` when the window closes, and restored on the next start. A
  minimized window comes back restored, or maximized if it was maximized before.
- The inspector is always shown. View › Log toggles the log pane.
- Minimum window size is about 1100×680. The mockup draws the window at 1280×800, with a 250 px left panel and
  a 290 px inspector.

## Vocabulary

The UI uses its own words for the core statuses.

| Core status / flag | UI word                              | Icon                               |
|--------------------|--------------------------------------|------------------------------------|
| `downloaded`       | Synced                               | green circle with check            |
| `pending`          | Waiting                              | hollow gray circle                 |
| (downloading)      | Waiting, shown as `Downloading 62%`  | blue progress ring that fills      |
| `failed`           | Failed                               | red circle with cross              |
| `match.review`     | Needs review                         | amber warning triangle             |
| `orphaned`         | Removed                              | orange circle with slash           |
| `kept`             | Kept                                 | gray pin                           |

In the Status column, Needs review wins over Synced and shows ⚠, because review is the state the user can act
on.

**Tracks and sources.** A row in the UI is a *track*: one file in the library. A track has one or more *sources*,
which are YouTube Music videos or Spotify tracks. The same song in a YouTube Music playlist and a Spotify playlist
is merged into one track, as long as the Matcher picked the video the YouTube Music playlist lists (see
`arch.md`, Merging). A track with a YouTube Music source never needs review, because the playlist owner chose
its video.

## Menu bar and toolbar

Each menu lists its items top to bottom; ─ is a separator. Shortcuts are right-aligned in the menu.

| Menu  | Items |
|-------|-------|
| File  | Add playlist… (Ctrl+N), Remove playlist…, ─, Open library folder, ─, Exit (Ctrl+Q) |
| Sync  | Sync all (F5), Sync selected playlist, ─, Cancel (Esc while syncing) |
| Track | Confirm match, Choose video…, Edit track… (Enter, double-click), ─, Retry, Keep, Move to Recycle Bin…, ─, Show in folder, Open on YouTube, Open source page |
| View  | Log (Ctrl+L), ─, Columns… |
| Tools | Settings… (Ctrl+Alt+S), Check for updates, ─, Rebuild library from files…, Library folder moved…, ─, Open log folder |
| Help  | About WestieLib |

Access keys: the menu bar has **F**ile, **S**ync, **T**rack, **V**iew, T**o**ols and **H**elp (Track and Tools can't
share T). The items, in order: File **A**dd, **R**emove, **O**pen, E**x**it; Sync **A**ll, **S**elected, **C**ancel;
Track **C**onfirm, C**h**oose, **E**dit, **R**etry, **K**eep, **M**ove, **S**how, **Y**ouTube, source **P**age;
View **L**og, **C**olumns; Tools **S**ettings, **U**pdates, **R**ebuild, **M**oved, **L**og folder; Help **A**bout.

The toolbar has `+ Add playlist`, `⟳ Sync all ▾`, `■ Cancel` and `⚙ Settings`.

- It is a white strip, 34 px tall including a 1 px `#D9D9D9` rule along its bottom, and as wide as the window.
- The buttons are flat, 26 px tall and centered in the strip, starting 4 px from its left edge. Each has 8 px on
  either side, a 16 px icon, 5 px, then its text. Buttons sit 1 px apart. A separator between Cancel and Settings
  takes 11 px: a 1 px `#D5D5D5` line, 20 px tall and centered, with 5 px on either side.
- A disabled button shows its icon in grayscale at 45% opacity, and its text in `#A0A0A0`, antialiased like all other
  text.
- The strip grows when the system font needs taller buttons.

- `Sync all ▾` is a split button. Its dropdown lists "Sync selected playlist" and then every playlist by name.
- The menu bar, the toolbar and the shortcuts share one set of rules, so a command is enabled everywhere or nowhere:
  - `Cancel` is enabled only while a sync runs. `Sync all` and `Sync selected playlist` are disabled during that
    time, and until every tool is installed.
  - `Remove playlist…` and `Sync selected playlist` need a selected playlist.
  - Track items need a selected track.
  - Everything else is always enabled.
- View › Log shows a check mark while the log pane is shown.

## Infobar

A yellow strip under the toolbar, in the Office/Edge style. It holds an icon, a message, action buttons and ✕.
Only one message shows at a time, in this priority order:

| Message                                                 | Actions                                                 |
|---------------------------------------------------------|---------------------------------------------------------|
| "5 tracks are no longer in any playlist."               | `Review…` selects All tracks and the Removed status tab |
| "Couldn't fetch 'Slow & bluesy'. It may be private or deleted." | `Details` opens the log                         |
| "The library folder D:\Music\WestieLib is missing."     | `Locate…` relocates it                                  |
| "WestieLib 1.3.0 is available." (when self-update can't install) | `Download page`                                |

✕ hides the current message until its cause changes. A dismissed fetch failure still shows ⚠ on its playlist
row.

## Left panel

The left panel is a list with two kinds of rows.

**All tracks** (glyph ▤): `All tracks`, then `100 tracks · 92 synced`. A separator follows it.

**Playlist rows** have variable height:

| Line | Content |
|------|---------|
| 1 | Source logo (the YouTube Music or Spotify icon, 14 px) and the name in semibold |
| 2 | `55 tracks` |
| 3 | synced/total bar, then `51/55` |
| 4 | non-zero counts only: `3 waiting · 1 failed · 2 to review` in gray / red / amber. Left out when all are zero |
| 5 | status icon, last-sync text, sync control (see below) |

**Line 5** reads like `✓ Synced today 14:03 ↻`:

- **Status icon.** It starts the line, flush with the progress bar above, and the text follows it. It is the
  first rule that applies, top to bottom:

  | Icon | Status       | Shown when                |
  |------|--------------|---------------------------|
  | ⚠    | fetch failed | the last fetch failed     |
  | ✕    | failures     | some tracks failed        |
  | ○    | unsynced     | some tracks are waiting   |
  | ✓    | synced       | none of the above         |

- **Last-sync text.** One of `Synced today 14:03`, `Synced yesterday`, `Synced 12 Oct`, `Never synced`, or
  `Couldn't fetch · synced 9 Oct` (with `Couldn't fetch` in amber). It ends in an ellipsis rather than pushing
  the control out of the row.
- **Sync control.** It sits directly after the text: a 20 px flat button with a gray ↻ glyph and no frame.
  - On hover it gets the light-blue hot frame and the tooltip "Sync <playlist name>". Clicking it syncs that
    playlist only.
  - While the playlist syncs, the line reads `Syncing… 6 of 22 ⟳`. The button turns into the blue ⟳, which
    spins clockwise at about one turn per 1.2 s, and the status icon steps aside because the sync supersedes it.
  - A playlist that is in the run but hasn't started yet reads `Queued ⟳`, with the icon still and faded.

An `+ Add playlist` button closes the list.

A click scopes the main panel to that playlist. The context menu offers Sync, Open in browser, Copy URL and
Remove playlist….

## Main panel

### Filter bar

- **Search**: an edit with the cue banner "Search title, artist, album". It matches title, artists, album and
  file name, case- and accent-insensitive through `TextNormalizer`.
- **Source**: a drop-down list with All, YouTube Music and Spotify.
- **Clear**: enabled when the search, the source or the status tab is not at its default.

### Status filter

A strip of flat buttons with counts. Exactly one button is selected. The counts take the scope, the search and
the source into account, so each button shows how many rows it would list:

`All │ Synced │ Waiting │ Needs review │ Failed │ Removed │ Kept`

- `Removed` and `Kept` appear only when their count is not zero.
- All = Synced + Waiting + Failed + Removed + Kept.
- Needs review is a subset of Synced.

### Result line

`Showing 8 of 100 · in WCS` gives the rows shown, out of all tracks in the library, followed by the scope. In
the All tracks scope, the `· in …` part is left out.

### Track table

A virtual list view in report mode. It has full-row select, the Explorer theme and multi-select. Columns sort
when you click their header. Right-click the header to show, hide or reorder columns.

| Column    | Default | Content |
|-----------|---------|---------|
| #         | playlist scope only | the entry's position in the playlist. It keeps its value when rows are filtered out, so a search can show 2, 5, 8. All tracks has no # column, because a position only means something inside a playlist |
| Status    | on      | status icon only (56 px). A downloading row shows a blue progress ring instead. Hovering the cell shows the word as a tooltip (`Synced`, `Needs review`, `Downloading 62%`, …) |
| Title     | on      | |
| Artists   | on      | joined with `, ` |
| Album     | on      | empty for waiting YouTube Music tracks, whose album comes from the info JSON at download time |
| Playlists | on      | names of every playlist that lists one of the track's sources, joined with `, `, ending in an ellipsis when cut |
| Time      | on      | m:ss |
| Source    | off     | logo of each source |
| Match     | off     | confidence %, `Override` or `Direct` |
| File      | off     | file name |

Default sort is Title in All tracks and # in a playlist scope. The row context menu is the Track menu.

## Inspector

**One track selected**, top to bottom:

- **Header**: cover art (square, 144 px), **title**, artists, `Album · 2021`. The cover comes from the file;
  tracks without a file show a placeholder.
- **Status**: the status icon and word, plus one line explaining it.
  - Failed tracks add a red reason line and a box with the last two lines of output (Consolas). There are two
    reasons: `yt-dlp exited with code 1`, or `No match on YouTube Music`.
  - Needs review tracks replace the explanation with an amber note: "Low-confidence match. Listen to the file,
    then confirm it or choose another video."
  - The **actions** that apply come right under that, so they stay above the fold:

    | State          | Actions                                 |
    |----------------|-----------------------------------------|
    | Needs review   | `Confirm match`, `Choose video…`, `Edit track…` |
    | Failed, no match | `Choose video…`, `Retry`              |
    | Failed, other  | `Retry`, `Edit track…`                  |
    | Synced, Waiting, Kept | `Edit track…`                    |
    | Removed        | `Keep`, `Move to Recycle Bin…`          |
- **Source** (titled **Sources** for a merged track):
  - one line per source: logo, name, and `Open track` / `Open`;
  - the playlists, as links;
  - the track key;
  - the ISRC.
- **Audio**:
  - for matched tracks: `Matched` (the video title), `Channel`, and `Length`, the video's length next to
    the source length; it shows in amber when the two differ by more than 5 s, which often gives away a live
    version or a cover;
  - `Video`, as a link;
  - `Chosen by`: `Playlist · <name>`, `Matcher` or `Override`;
  - `Confidence`, as a bar + %; the bar uses the paused (yellow) state when review is needed;
  - for a merged track, `Spotify match: Same video · 96%` replaces the matcher rows.
- **File**: the file name and `Show in folder`.

**Several tracks selected**: `4 tracks selected`, a count per status, and the bulk actions that apply to all of
them, for example `Keep 4 tracks` and `Move 4 files to Recycle Bin…`.

**Nothing selected**:

- In a playlist scope it shows a playlist card with:
  - glyph and name, URL link, track count;
  - a synced/total bar with the counts;
  - how many tracks it shares with other playlists of the same source (shared tracks download once);
  - last sync;
  - `Sync playlist` and `Remove playlist…`.
- In All tracks it shows the library folder, totals and `Open library folder`.

The inspector scrolls vertically when its content is taller than the panel.

## Log pane

- Hidden on the first start; after that, it comes back as it was when the window closed. View › Log or Ctrl+L
  toggles it. It spans the full window width, between the panels and the status bar, and has its own splitter.
- Its mini toolbar has `Log`, flat buttons `All │ Warnings & errors`, a `Selected track only` check box,
  `Open log folder` and ✕.
- A list view with the columns Time | Level | Playlist / Track | Message. Warnings show in amber, errors in red.
  New lines scroll it to the bottom unless the user has scrolled up.
- The full tool output goes only to the log file, not to this list.

## Status bar

| Part    | Content |
|---------|---------|
| Run     | `Ready · last sync today 14:05`, or `Syncing 6 of 25` with an embedded progress bar |
| Library | `100 tracks · D:\Music\WestieLib` |
| Tools   | `Tools ✓`, `Installing tools… ffmpeg 45%`, `Updating tools…` or `Tools ⚠`; the tooltip lists the releases |
| Version | `v1.2.0` |

- The status bar is the native one, 23 px tall, or taller when the system font needs it. It ends in a size grip.
- Run, Library and Tools are 270, 300 and 220 px wide, from the left. Version is 80 px wide against the right edge,
  with its text centered between its separator and the grip, or the window edge when maximized. No separator
  follows it. The space between Tools and Version stays empty.
- Texts keep 8 px from the edges of their part, and end in an ellipsis when they don't fit.

## Dialogs

Modal dialogs follow native behavior: the main window turns inactive, and nothing is dimmed.

- **Add playlist**:
  - "Paste a link to a public YouTube Music or Spotify playlist."
  - A URL edit, with a live detection line under it: the YouTube Music or Spotify logo with
    `YouTube Music playlist` / `Spotify playlist`, or `Not a supported playlist link`.
  - A `Sync now` check box (on by default).
  - `Add` (default, disabled until the link is supported) and `Cancel`.
- **Choose a video**. Opened by `Choose video…` in the inspector, the Track menu or Edit track. One dialog
  covers both the case with several candidates and the case where nothing was found:
  - At the top: the source track (glyph, title, artists, album, length) and either "Current match: <title>,
    58% confidence" or "The Matcher found no usable video."
  - `Search YouTube Music:` an edit pre-filled with the Matcher's query, a `Songs and videos` / `Songs` /
    `Videos` drop-down, and `Search`.
  - A results list with Title | Artist | Album | Type | Length:
    - the current match is marked `(current)`;
    - a video that already belongs to another track is marked `(in library)`, and choosing it merges the two
      tracks;
    - lengths more than 5 s off the source length are amber;
    - with no results, the list shows "No results. Try fewer words, or paste a link below."
  - `Or paste a YouTube or YouTube Music link:` an edit with a live detection line (YouTube logo, `YouTube video ·
    <id>`).
  - The footer has `Open in browser` on the left (it plays the selected result or the pasted link), then
    `Use this video` (default) and `Cancel`.
  - `Use this video` sets the video override and downloads the track again.
- **Edit track**. Opened by `Edit track…`, Enter, or a double-click on a row:
  - **Cover art**: a 132 px preview, `Choose image…`, `Paste` and `Reset`, plus a source line (`From the video
    thumbnail`, `From Spotify`, `Custom image`).
  - **Tags**: Title, Artists, Album and Year edits. A changed field gets a `Reset` link, and the original value
    shows under it in gray.
  - **Audio**: `YouTube link:` edit and `Choose video…`, with the note "A different link downloads the audio
    again."
  - `Save` (default) and `Cancel`. Saving re-tags the file and renames it when the artists or title changed.
    Edited values survive later syncs.
- **Settings**, as group boxes:
  - **Library**: a read-only folder edit and `Change…`.
  - **Files**: `Format:` radio buttons Opus / FLAC, and an `Embed album art in files` check box (on by default).
    Both apply to new downloads; existing files stay as they are.
  - **Downloads**: `Parallel downloads` spin (1–4) and `Pause between tracks` spin (seconds).
  - **Tools**: a list of tool, release and status. Under it, an `Update tools automatically` check box (on by
    default), `Check for updates`, and "Last checked …".
  - **Maintenance**: `Rebuild library from files…`.
  - The footer shows the version on the left, and `OK` / `Cancel`.
- **About** is a TaskDialog with the app icon, `WestieLib` as the main instruction, `Version 1.2.0` (the build's
  version) and OK.
- **Confirmations** use a TaskDialog:
  - Remove playlist says how many tracks become Removed.
  - Move to Recycle Bin says how many files.
  - Rebuild says it replaces track entries.
- **First run** has no playlists. The main panel shows an empty state:
  - a welcome line;
  - the library folder with `Browse…`;
  - `+ Add your first playlist`.

  Tools install in the meantime, and the status bar shows their progress.

## Command and event mapping

| Core API | UI |
|----------|----|
| `addPlaylist` | toolbar, left panel button, File menu, first-run button → Add playlist dialog |
| `removePlaylist` | playlist context menu, File menu, inspector playlist card → confirmation |
| `sync` / `cancel` | toolbar split button, Sync menu, playlist row sync button, playlist context menu, inspector card |
| `resolveRemovals` | infobar → Removed tab, Track menu, inspector (single and bulk) |
| `retry` | inspector Retry, Track menu |
| `confirmMatch` | inspector actions, Track menu |
| `setVideoOverride` | Choose a video dialog, Edit track dialog (Audio link) |
| `editTrack` | Edit track dialog (tags, cover art) |
| `searchVideos` / `VideoSearchResults` | Choose a video dialog |
| `setSettings` | Settings dialog OK |
| `relocateLibrary` / `rebuildState` / `checkForUpdates` | Tools menu, Settings, infobar |
| `TrackStatusChanged` / `TrackProgress` | Status cell, status counts, playlist row, inspector, status bar |
| `PlaylistFetched` / `PlaylistFetchFailed` | playlist row (name, count, `Couldn't fetch`), infobar, log |
| `TrackUpdated` | table row, inspector |
| `TracksMerged` | the deleted track's row disappears; the selection and inspector move to the surviving track |
| `RemovalsPending` | infobar, Removed tab |
| `ToolStatus` / `AppUpdate` | status bar, infobar, Settings |
| `LogLine` | log pane |

## Win32 control mapping

| Element | Implementation |
|---------|----------------|
| Frame | HMENU + accelerator table; three custom splitters |
| Toolbar | `TOOLBARCLASSNAME` with `TBSTYLE_FLAT \| TBSTYLE_LIST` and an image list; `BTNS_DROPDOWN` + `TBSTYLE_EX_DRAWDDARROWS` for Sync all |
| Infobar | custom-painted child window holding push buttons |
| Status bar | `STATUSCLASSNAME` with parts and an embedded `PROGRESS_CLASS` |
| Left panel | `LISTBOX` with `LBS_OWNERDRAWVARIABLE`; rows, bars and icons drawn in `WM_DRAWITEM`. The per-row sync control is drawn in the same pass, right after the measured line-5 text (`GetTextExtentPoint32`, `DrawThemeBackground` toolbar button part for the hot state). It is hit-tested in `WM_LBUTTONDOWN`, hot-tracked with `WM_MOUSEMOVE` + `TrackMouseEvent`, and has a tooltip-control tool per row rectangle. The spinning sync icon is a 12-frame image list (30° steps) advanced by a ~100 ms `WM_TIMER` that invalidates only the icon rectangles. The timer runs only while a sync is active. When `SPI_GETCLIENTAREAANIMATION` is off, the icon stays still. The inspector's `Downloading` icon uses the same frames |
| Filter bar | `EDIT` with `EM_SETCUEBANNER`, a `CBS_DROPDOWNLIST` combo box, a push button |
| Status filter | `WC_TABCONTROL` with `TCS_BUTTONS \| TCS_FLATBUTTONS` and `TCS_EX_FLATSEPARATORS`; labels rewritten with counts |
| Track table, log | `WC_LISTVIEW` with `LVS_REPORT \| LVS_OWNERDATA`, `LVS_EX_FULLROWSELECT \| LVS_EX_DOUBLEBUFFER \| LVS_EX_HEADERDRAGDROP`; `NM_CUSTOMDRAW` for the Status icon and colored text; `SetWindowTheme(L"Explorer")`. The progress ring comes from a 24-step image list. The Status tooltip uses `LVS_EX_INFOTIP` + `LVN_GETINFOTIP`. The # column is inserted when a playlist is selected and removed for All tracks |
| Inspector | scrollable child window with statics, `WC_LINK`, buttons, edits, `SS_ETCHEDHORZ` section rules; covers decoded with WIC into an HBITMAP |
| Dialogs | `DIALOGEX` resources, `UPDOWN_CLASS`, `BS_AUTORADIOBUTTON`, `IFileOpenDialog` (`FOS_PICKFOLDERS` for the library, image filter for covers), `TaskDialogIndirect`. The Choose a video results are a report ListView, and its empty text comes from `LVN_GETEMPTYMARKUP`. Edit track's cover is an `SS_BITMAP` static, and Paste reads `CF_DIB` |
| Global | ComCtl32 v6 manifest, Per-Monitor V2 DPI awareness, message font from `SPI_GETNONCLIENTMETRICS` |

The UI uses the Windows 10 palette:

| Role | Color |
|------|-------|
| window | `#FFFFFF` |
| face | `#F0F0F0` |
| selection | `#CCE8FF`, border `#99D1FF` |
| pressed button | `#CCE4F7`, border `#005499` |
| accent | `#0078D7` |
| gray text | `#6D6D6D` |
| progress | `#06B025` |
| infobar | `#FFF4CE` |

Nothing in the UI uses rounded cards, shadows or gradients.

**Brand logos.** The YouTube Music, Spotify and YouTube icons ship as icon resources made from the official
brand assets; the mockup's versions are drawn approximations.
