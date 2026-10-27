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
├────────────────────────┤ Showing 214 of 412 in WCS                       │                │
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
│ Syncing 12 of 40 ▓▓▓▓░░░ │ 412 tracks │ D:\Music\WestieLib │    │ Tools ✓ │ v1.2.0 │ status bar
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
- The window placement (normal bounds and maximized state), the panel widths, the log height, whether the log
  shows and the track table's column widths are saved under `HKCU\Software\WestieLib\Window` when the window closes,
  and restored on the next start. A minimized window comes back restored, or maximized if it was maximized before.
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
| (needs a tool)     | Waiting, shown as `Waiting for ffmpeg` | gray hourglass, still            |
| `failed`           | Failed                               | red circle with cross              |
| `match.review`     | Needs review                         | amber warning triangle             |
| `orphaned`         | Removed                              | orange circle with slash           |
| `kept`             | Kept                                 | gray pin                           |
| (a playlist's later entry of a track) | Duplicate         | two overlapping gray squares, nearly square-cornered |

In the Status column, Needs review wins over Synced and shows ⚠, because review is the state the user can act
on. Duplicate wins over all of them, but only in the Status column and the inspector's status line: a duplicate
counts under its track's status everywhere else.

**Waiting for tools.** Work that needs a tool waits while that tool isn't installed, and only then; work that doesn't
need it goes ahead. Waiting isn't failing: the hourglass is gray and still, unlike ✕ for a failed download and ⚠ for
something the user should act on, and the work starts by itself once the tool is installed. A tool whose install
failed shows in the status bar (see Status bar › Tools) and in Settings, not on the tracks waiting for it.

| Work                                      | Needs           |
|-------------------------------------------|-----------------|
| Look up a Spotify playlist (Add playlist) | –               |
| Look up a YouTube Music playlist (Add playlist) | yt-dlp    |
| Fetch a Spotify playlist                  | –               |
| Fetch a YouTube Music playlist            | yt-dlp          |
| Match a Spotify track (search YouTube Music) | –            |
| Download a track: audio, thumbnail cover  | yt-dlp, ffmpeg  |
| Choose a video: search                    | yt-dlp          |

A track that waits shows the hourglass in the Status column, with `Waiting for ffmpeg` (or `Waiting for yt-dlp and
ffmpeg`) as its tooltip and in the inspector. A playlist whose fetch waits shows it on line 5 (see Left panel). The
Add playlist dialog shows it on its detection line (see Dialogs).

**Tracks, sources and entries.** A *track* is one file in the library. A track has one or more *sources*, which
are YouTube Music videos or Spotify tracks. Sources with the same video are merged into one track (see `arch.md`,
Merging): the same song in a YouTube Music and a Spotify playlist, or one recording a Spotify playlist lists under
several ids (single, album, compilation). All tracks shows a row per track. A playlist shows a row per *entry*: one
source it lists. So a merge never takes a row out of a playlist: when a playlist lists several sources of one track,
the first is the original and the later ones are *duplicates*, which download nothing of their own and share its
file. Entries in different playlists are never duplicates of each other. A track with a YouTube Music source never
needs review, because the playlist owner chose its video.

## Menu bar and toolbar

Each menu lists its items top to bottom; ─ is a separator. Shortcuts are right-aligned in the menu.

| Menu  | Items |
|-------|-------|
| File  | Add playlist… (Ctrl+N), Remove playlist…, ─, Open library folder, ─, Exit (Ctrl+Q) |
| Sync  | Sync all (F5), Sync selected playlist, ─, Cancel |
| Track | Confirm match, Choose video…, Edit track… (Enter, double-click), ─, Retry, Keep, Move to Recycle Bin…, ─, Show in folder, Open on YouTube, Open source page |
| View  | Log (Ctrl+L), ─, Columns… |
| Tools | Settings… (Ctrl+Alt+S), Check for updates, ─, Rebuild library from files…, Library folder moved…, ─, Open log folder |
| Help  | About WestieLib |

Access keys: the menu bar has **F**ile, **S**ync, **T**rack, **V**iew, T**o**ols and **H**elp (Track and Tools can't
share T). The items, in order: File **A**dd, **R**emove, **O**pen, E**x**it; Sync **A**ll, **S**elected, **C**ancel;
Track **C**onfirm, C**h**oose, **E**dit, **R**etry, **K**eep, **M**ove, **S**how, **Y**ouTube, source **P**age;
View **L**og, **C**olumns; Tools **S**ettings, **U**pdates, **R**ebuild, **M**oved, **L**og folder; Help **A**bout.

The menu bar is white and runs straight into the toolbar, without the gray line Windows draws under a menu bar. Its
items look as usual: the same font, hover and open highlights, and gray text while the window is inactive.

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
  - `Cancel` is enabled only while a sync runs. `Sync all` is enabled when there is at least one playlist and no sync
    runs. The tools don't matter: work that needs a missing tool waits (see Vocabulary › Waiting for tools).
  - `Remove playlist…` and `Sync selected playlist` need a selected playlist, and `Sync selected playlist` also needs
    no sync running.
  - Track items need a selected track.
  - `Check for updates` is disabled while tools are checked, installed or updated, and while a sync runs.
  - Everything else is always enabled.
- View › Log shows a check mark while the log pane is shown.

## Text fields

Every text field deletes words as other Windows apps do. Ctrl+Backspace deletes the word before the caret, and
Ctrl+Delete the word after it; with a selection, both delete the selection. A word is a run of letters and digits, or a
run of other characters, and the spaces next to it go with it: in `C:\Music\Westie`, Ctrl+Backspace takes `Westie`,
then `\`, then `Music`. Enter in a field outside a dialog doesn't beep.

Each field keeps its own history: Ctrl+Z undoes the last change and Ctrl+Y redoes it. The history is a line, so a
change after an undo drops the changes that could have been redone. Characters typed one after another are one change,
as are Backspaces or Deletes one after another; moving the caret, or any other kind of change, starts a new one. An undo
puts back the text and the selection from before the change. Text the app sets, as when it loads the library folder or
Clear all empties the search, starts a new history. The shortcuts are the only way in: Alt+Backspace does nothing.

Text fields have no context menu; a right-click or the Menu key does nothing.

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

So far it shows only fetch failures, one at a time: the first playlist, in the order they were added, whose last fetch
failed and wasn't dismissed. A later failure of a dismissed playlist shows again. `Details` shows the log pane, which
lists nothing yet; its lines come with the log.

```
│ ⚠  Couldn’t fetch “Slow & bluesy”. It may be private or deleted.                         [Details]  ✕ │
```

The strip is `#FFF4CE` with a 1 px `#EADB9F` rule along its bottom. From the left: 10 px, the 16 px ⚠, 8 px, the
message (an ellipsis when it doesn't fit), 8 px, `Details` (a compact button), 8 px, and ✕ in a 24 px box, 6 px from
the right edge. A click on the box dismisses the message.

## Left panel

The left panel has the All tracks row at the top, a separator under it, and then the list of playlists. All tracks
stays where it is; only the playlist list scrolls.

**All tracks** (glyph ▤): `All tracks`, then `100 tracks · 92 synced`. It is selected while the main panel shows the
whole library, which is always the case until there are playlists.

With no playlists, the list reads `No playlists yet.` in gray.

**Playlist rows** have variable height:

| Line | Content |
|------|---------|
| 1 | Source logo (the YouTube Music or Spotify icon, 14 px) and the name in semibold. The name comes from the look-up in Add playlist and is refreshed by every fetch. A playlist stored without a name (by an earlier version) reads `YouTube Music playlist` or `Spotify playlist` until its first fetch |
| 2 | `55 tracks` |
| 3 | synced/total bar, then `51/55` |
| 4 | non-zero counts only: `3 waiting · 1 failed · 2 to review` in gray / red / amber. Left out when all are zero |
| 5 | status icon, last-sync text, sync control (see below) |

**Line 5** reads like `✓ Synced today 14:03 ↻`:

- **Status icon.** It starts the line, flush with the progress bar above, and the text follows it. It is the
  first rule that applies, top to bottom:

  | Icon | Status            | Shown when                                          |
  |------|-------------------|-----------------------------------------------------|
  | ⚠    | fetch failed      | the last fetch failed                               |
  | ✕    | failures          | some tracks failed                                  |
  | ⧗    | waiting for tools | its fetch, or some of its tracks, wait for a tool   |
  | ○    | unsynced          | some tracks are waiting, or it was never synced     |
  | ✓    | synced            | none of the above                                   |

  While it waits for tools, the last-sync text reads `Waiting for yt-dlp`.

- **Last-sync text.** One of `Synced today 14:03`, `Synced yesterday`, `Synced 12 Oct`, `Never synced`, or
  `Couldn't fetch · synced 9 Oct` (with `Couldn't fetch` in amber). It ends in an ellipsis rather than pushing
  the control out of the row.
- **Sync control.** It sits directly after the text: a 20 px flat button with a gray ↻ glyph and no frame.
  - On hover it gets the light-blue hot frame and the tooltip "Sync <playlist name>". Clicking it syncs that
    playlist only.
  - While the playlist syncs, the line reads `Syncing… 6 of 22 ⟳`. The button turns into the blue ⟳, which
    spins clockwise at about one turn per 1.2 s, and the status icon steps aside because the sync supersedes it.
  - A playlist that is in the run but hasn't started yet reads `Queued ⟳`, with the icon still and faded.

The playlists are listed in the order they were added. A playlist that was just added reads `0 tracks`, an empty bar
with `0/0`, and `○ Never synced ↻`.

**So far** a sync fetches playlists, YouTube Music and Spotify alike, and then matches the Spotify tracks: their
tracks show as Waiting, a track whose match fails as Failed, and nothing downloads. Meanwhile:

- the playlist being fetched reads `Syncing…`, without `6 of 22`, which counts downloads;
- a YouTube Music playlist whose fetch waits for yt-dlp shows ⧗ and `Waiting for yt-dlp`, with the still, faded ⟳ of
  a queued one. A Spotify fetch needs no tool, so it never waits;
- a Spotify playlist's tracks show 100 at a time while it is fetched: the row's counts, the table and the cards
  update with every page. A fetch that fails or is cancelled halfway keeps the tracks it read, beside the ones the
  playlist had; the next fetch that succeeds settles the list;
- a fetch that succeeds sets the last sync, so a playlist whose tracks all wait reads `○ Synced today 14:03 ↻`;
- each match updates the table, the cards and the rows' counts as it ends. A matched track that merges into one
  with the same video leaves the table; when it was selected, the selection is gone with it.

An `+ Add playlist` button closes the list. It and `No playlists yet.` line up with the text of the rows above them,
so their left edge is where the All tracks name starts.

**Selection.** Exactly one item is selected: All tracks or one playlist. The rows react to the mouse as in Explorer's
navigation pane:

| Look     | When                                                         | Fill      | Frame     |
|----------|--------------------------------------------------------------|-----------|-----------|
| hover    | the mouse is over a row that isn't selected                  | `#E5F3FF` | `#E5F3FF` |
| pressed  | the left button went down on the row and the mouse is still over it | `#CCE8FF` | `#99D1FF` |
| selected | the row is selected, hovered or not                          | `#CCE8FF` | `#99D1FF` |

- The colors don't change with the keyboard focus.
- A click on All tracks or a playlist row selects it when the button comes up over the row it went down on, which
  scopes the main panel and the inspector to it. While the button is held, only that row reacts: moving off it drops
  the pressed look, moving back brings it back, and releasing elsewhere selects nothing. A click between or below the
  rows changes nothing.
- The hover follows the rows when the list scrolls under the mouse.
- A click also gives the playlist list the keyboard focus. Up and Down move the selection one item, Home and End to
  All tracks and the last playlist, PgUp and PgDn by as many rows as the list shows. All tracks counts as the item
  before the first playlist. The selected row scrolls into view.
- Adding a playlist selects it. When the selected playlist goes away, All tracks is selected.
- Every start begins on All tracks; the selection isn't saved.

**Context menu.** A playlist row's context menu has **S**ync, **O**pen in browser, **C**opy URL, ─, **R**emove
playlist….

- A right-click selects the row under the mouse first, and gives the list the focus. A right-click between or below
  the rows, or on All tracks, shows no menu.
- Shift+F10 and the menu key open it for the selected playlist, at the row's bottom-left corner, while the list has
  the focus. With All tracks selected they show nothing.
- Its items follow the command rules: they need a selected playlist, and Sync also needs no sync running. Sync does
  what `Sync selected playlist` does.
- **Open in browser** opens the playlist's URL in the default browser, as the playlist card's link does. **Copy URL**
  puts the URL on the clipboard; when another program holds the clipboard, it beeps instead.

**Remove playlist…** (File menu, context menu, playlist card) asks first (see Dialogs › Confirmations). After a
removal All tracks is selected; removing the last playlist brings back the welcome screen and the inspector's
placeholder.

**Open library folder** (File menu, Library card) opens the library folder in Explorer, or the nearest folder above it
that exists while the first sync hasn't created it yet. When even the drive is missing it does nothing. When Windows
can't open a folder or a link, it says so in its own message.

## Main panel

### First run

While there are no playlists, the main panel shows a welcome screen instead of the filter bar, the status filter and
the track table. Its content is a column 430 px wide, or the panel's width less 16 px on each side when that is
narrower. The column is centered in the panel both ways, and starts at the top when the panel is too short for it.
From the top:

- the app icon, 40 px;
- `Welcome to WestieLib`, semibold at 1.5 times the message font, 12 px below the icon;
- 6 px below, in gray: "WestieLib keeps your YouTube Music and Spotify playlists in sync with a folder of tagged audio
  files. Choose where the files go, then add your first playlist.";
- 18 px below, `Library folder`, and 4 px under that an editable field with the folder, then `Browse…` 8 px to its
  right;
- 4 px under the field, its hint line (below);
- 16 px below, `+ Add your first playlist`, the default button. It opens the Add playlist dialog.

The library folder:

- starts as `Music\WestieLib` in the user's profile, the *default folder*. It doesn't have to exist; the first sync
  creates it.
- The hint line under the field says what the text in the field names, and changes as the user types:

  | The text | Hint |
  |----------|------|
  | isn't a full path (`C:\…` or `\\server\share\…`), or has a character a folder name can't have | "Enter a full folder path, such as `<default folder>`." in red |
  | is on a drive letter this PC doesn't have | "That drive isn't on this PC." in red |
  | is a file | "That's a file, not a folder." in red |
  | is a folder that doesn't exist yet | "This folder doesn't exist yet. WestieLib creates it when it first syncs." in gray |
  | is a folder that exists, or one on a network drive or share | nothing. Network paths aren't looked up while typing, because an unreachable server would hang the window |

  The line keeps room for its longest hint, wrapped to the column, so nothing moves while the hint changes.
- Spaces around the text are dropped, and so are the quotes Explorer's "Copy as path" puts around a path. A trailing
  backslash goes too.
- Typed text takes effect when the field loses focus, and when the window closes, if its hint is gray or empty. With
  a red hint the previous folder stays in effect, and the text and the hint stay for the user to fix.
- `Browse…` opens a folder picker titled "Library folder", at the typed folder if the text is a full path and else at
  the folder in effect, or at the nearest folder above it that exists. A chosen folder takes effect at once.
- The status bar's Library part shows the folder in effect.

### Filter bar

- **Search**: an edit with the cue banner "Search title, artist, album". It matches title, artists, album and
  file name, case- and accent-insensitive through `TextNormalizer`. The text splits into words at spaces, and a row
  shows when each word appears somewhere in those fields, in any order: `ana late` finds Late Night Swing by Ana, Ben.
  So far no track has a file, so the search matches the other three. Esc empties the field. Ctrl+F (not in a menu)
  puts the focus in it and selects its text. A 16 px magnifier sits inside the field, 5 px from its left edge, and the
  text starts 5 px after it.
- **Source**: the label `Source`, its text level with the search field's, then a drop-down list with All, YouTube
  Music and Spotify.
- **Clear**: a compact button (no 75 px minimum, 9 px of padding at the sides), enabled when the search, the source
  or the status tab is not at its default. It empties the search, sets the source to All and selects the All tab.

The bar has 8 px of padding at the top and sides and 4 px under it, with 8 px between its parts. The search field is
300 px wide, and narrows when the panel is too narrow for it; the drop-down is 128 px wide.

### Status filter

A strip of flat buttons with counts. Exactly one button is selected. The counts take the scope, the search and
the source into account, so each button shows how many rows it would list:

`All │ Synced │ Waiting │ Needs review │ Failed │ Duplicates │ Removed │ Kept`

- Each button reads its word and its count, as in `Needs review 3`. The buttons are 22 px tall with 8 px at the
  sides of the text. The strip has 4 px above it, 8 px at the sides and 2 px under it. The selected button has the
  pressed button colors, a button under the mouse the hover color (border `#CCE8FF`), and a 1×14 px `#C9C9C9` line
  separates the buttons.
- `Removed` and `Kept` appear only when their count is not zero, or while they are selected.
- `Duplicates` lists the playlist's duplicates. It shows in every playlist scope, also at 0, and never in All tracks,
  which has none.
- All = Synced + Waiting + Failed + Removed + Kept.
- Needs review is a subset of Synced, and Duplicates of All: a duplicate also counts under its track's status.

### Result line

`Showing 8 of 100 in WCS` gives the rows shown, out of all tracks in the library, followed by the scope. In
the All tracks scope, the `in …` part is left out. It is gray, with 3 px above it, 9 px at the sides and 5 px
under it. A 1 px rule (`#E5E5E5`) separates it from the table.

### Track table

A virtual list view in report mode. It has full-row select, the Explorer theme and multi-select. Columns sort
when you click their header. Right-click the header to show, hide or reorder columns.

| Column    | Default | Content |
|-----------|---------|---------|
| #         | playlist scope only | the entry's position in the playlist. It keeps its value when rows are filtered out, so a search can show 2, 5, 8. All tracks has no # column, because a position only means something inside a playlist |
| Status    | on      | status icon only (56 px). A downloading row shows a blue progress ring instead, and a duplicate the duplicate icon. Hovering the cell shows the word as a tooltip (`Synced`, `Needs review`, `Downloading 62%`, …); a duplicate's reads `Duplicate of track #3 · <its track's word>`, with the original's position |
| Title     | on      | |
| Artists   | on      | joined with `, ` |
| Album     | on      | Spotify tracks have it from the fetch. Empty for waiting YouTube Music tracks, whose album comes from the info JSON at download time, unless a Spotify source merged in has one. Title, Artists, Album and Time show the track's effective metadata (`arch.md`), so a duplicate reads like its original |
| Playlists | on      | names of every playlist that lists one of the track's sources, joined with `, `, ending in an ellipsis when cut |
| Time      | on      | m:ss |
| Source    | off     | logo of the row's source: the entry's, or in All tracks the track's first entry's |
| Match     | off     | confidence %, `Override` or `Direct` |
| File      | off     | file name |

Rows are 22 px tall. A 1 px rule (`#E5E5E5`) runs under the column titles, across the table's width. The default widths are # 34, Status 56, Title 204, Artists 156, Album 132, Playlists 114 and
Time 48 px; # and Time are right-aligned. A column the user resizes keeps its width in every scope and across
restarts; # keeps its width while All tracks hides it.

Default sort is Title in All tracks and # in a playlist scope. A click on a header sorts by that column, ascending, and
a second click reverses it, however quickly it follows; the header shows the arrow. The sort is stable: rows that tie on
the column keep the order they had before the click, so sorting by Album after Artists sorts each album by artist. A
reverse turns the whole list round, ties included. Rows that tie on every column the user sorted by go by the scope's
default sort, then track key. The order holds while the rows are filtered or refreshed. Another scope starts at its
default sort, and forgets the earlier clicks. The row context menu is the Track menu.

The selection stays with its tracks while the rows are sorted, searched, filtered by source or refreshed by a sync; a track that
is filtered out loses it. Selecting another status tab or another scope (All tracks or a playlist) clears it.

All tracks shows a row per track, for its first entry: in the left panel's playlist order, then by position. A
playlist scope shows a row per entry: each source the playlist lists, at its first position (a source it lists twice
shows once). A row whose track the playlist lists earlier through another source is a duplicate (see Vocabulary).
The status tabs, the result line, the playlist rows' counts and the selection card count rows, a duplicate under its
track's status; the library's counts count tracks.

A refresh, such as each fetched page or match of a sync, keeps the table where it is scrolled. Another scope, status
tab, search or source starts it at the top.

## Inspector

**One track selected**, top to bottom:

- **Header**: cover art (square, 144 px), **title**, artists, `Album · 2021`. The cover comes from the file;
  tracks without a file show a placeholder.
- **Status**: the status icon and word, plus one line explaining it.
  - Failed tracks add a red reason line and a box with the last two lines of output (Consolas). There are three
    reasons: `yt-dlp exited with code 1`, `No match on YouTube Music` (the box lists the searched queries), or
    `Couldn't search YouTube Music` (the box holds the error).
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
  - for matched tracks: `Matched` (the video title), `Artists` (the video's, as YouTube Music's search lists them),
    and `Length`, the video's length next to the source length; it shows in amber when the two differ by more than
    5 s, which often gives away a live version or a cover;
  - `ID`, the YouTube Music id as a link; `No match found` in red when the Matcher found nothing;
  - `Chosen by`: `Playlist`, `Matcher` or `Override`;
  - `Confidence`, as a bar + %; the bar uses the paused (yellow) state when review is needed;
  - for a YouTube Music entry whose track merged with a Spotify source, `Spotify match: Same ID · 96%` replaces
    the matcher rows.
- **File**: the file name and `Show in folder`.

**Several tracks selected**: `4 tracks selected`, a count per status, and the bulk actions that apply to all of
them, for example `Keep 4 tracks` and `Move 4 files to Recycle Bin…`.

So far no track has a file, so the track card shows no cover, no year, and `File` not downloaded yet. It shows the
status with its explanation, a failed match's reason and output, and the Audio rows of a video that a YouTube Music
source or the Matcher chose. Until Choose video exists, a failed match explains "No usable video was found. It's
searched again on the next sync." The Needs review note, the ISRC and every action button come with their
features, and so do the selection card's bulk actions.

```
┌ inspector ──────────────────────┐   ┌ inspector ──────────────────────┐
│          ┌──────────┐           │   │ 2 tracks selected               │
│          │          │           │   │                                 │
│          │    ♫     │           │   │ Status ──────────────────────── │
│          └──────────┘           │   │ Synced    ✓ 1                   │
│ Best Part                       │   │ Waiting   ◷ 1                   │
│ Daniel Caesar, H.E.R.           │   │                                 │
│ Freudian                        │   │                                 │
│                                 │   │                                 │
│ Status ──────────────────────── │   │                                 │
│ ◷ Waiting                       │   │                                 │
│ Downloads on the next sync.     │   │                                 │
│                                 │   │                                 │
│ Sources ─────────────────────── │   │                                 │
│ Listed on ● YouTube Music · Open│   │                                 │
│           ● Spotify · Open track│   │                                 │
│ Playlists WCS, Slow & bluesy    │   │                                 │
│ Key       ytm:5aM1xdk8lRw       │   │                                 │
│                                 │   │                                 │
│ Audio ───────────────────────── │   │                                 │
│ ID        Chosen when it downl… │   │                                 │
│                                 │   │                                 │
│ File ────────────────────────── │   │                                 │
│ Not downloaded yet.             │   │                                 │
└─────────────────────────────────┘   └─────────────────────────────────┘
```

The track card, from the top, in DIPs:

- **Cover**: 144 square, centered, then 12 px to the title. Without a file it's a `#F0F0F0` square with a 1 px
  `#E5E5E5` border, with a 28 px note glyph in its middle.
- **Title** in semibold at 125% of the message font, on 20 px lines, then the **artists** (`, ` between them) 1 px
  below on 16 px lines. Both wrap. Then the album line in gray, which ends in an ellipsis: the album and year
  (`Freudian · 2017`), or `Album is read when it downloads` when there's no album.
- **Status**: the section title as in the other cards, 7 px, then the status line: its 16 px icon (synced, waiting,
  warn, failed, removed or kept), 6 px, and the word in semibold, on a 16 px line. The explanation follows 3 px below
  in gray, and wraps:

  | Status       | Explanation |
  |--------------|-------------|
  | Synced       | The file is in the library and tagged. |
  | Waiting      | Downloads on the next sync. |
  | Failed       | The last attempt failed. It's retried on the next sync. A failed match says "No usable video was found. Choose one yourself." instead |
  | Removed      | No longer in any playlist. The file is still in the library. |
  | Kept         | Kept after it left every playlist. |
  | Duplicate    | Duplicate of track #12 in this playlist. (The original's position) |

  A duplicate's status line shows the duplicate icon and `Duplicate` whatever its track's status; a failure's reason
  and box still follow, since they are the track's.

  A failed track's reason follows 8 px below in red, on a 16 px line, then 4 px below the box: a 1 px `#C5C5C5`
  border, 4 px padding at the top and bottom and 6 px at the sides, and the last two lines of the error in Consolas
  at 11/12 of the message font's size, wrapped.

- **Source** (**Sources** when the track has more than one kind of source) holds rows like the other cards':
  - one row per kind of source the track has, YouTube Music first. Its key is `Source`, or with two kinds `Listed on`
    on the first row and nothing on the second. Its value is the logo (16 px), 6 px, the name, ` · `, and a link,
    `Open` for YouTube Music and `Open track` for Spotify, that opens the source's page in the browser
    (`https://music.youtube.com/watch?v=<id>`, `https://open.spotify.com/track/<id>`);
  - `Playlists`: the track's playlists, in the left panel's order, as links with `, ` between them. A click selects
    that playlist, as a click on its row does. A link that doesn't fit ends in an ellipsis, and the ones after it
    don't show. A track in no playlist reads `None` in gray;
  - `Key`: the row's source key.
- **Audio** holds rows like Source's, by how the row's source got its video, so a duplicate shows what the Matcher
  did with it:
  - a Spotify entry with a match: `Matched`, `Artists`, `Length`, `ID`, `Chosen by` `Matcher`, `Confidence`;
  - a YouTube Music entry: `ID`, `Chosen by` `Playlist`, and `Spotify match` `Same ID · 96%` when the
    track's first Spotify source was matched to that video;
  - otherwise by where the track's video comes from (`arch.md`, Video resolution), and with none, `ID` reads
    `Chosen when it downloads` in gray, or `No match found` in red after a failed match.

  `Length` is the video's length (`m:ss`), in amber when it differs from the Spotify length by more than 5 s, then
  ` · Spotify 3:32` in gray. `ID` is the YouTube Music id (the video id) as a link to `https://music.youtube.com/watch?v=<id>`.
  `Confidence` is a 110 px bar, 6 px tall and centered on the line, filled in green as far as the confidence, then
  6 px and the percentage; a narrower card shortens the bar.
- **File**: `Not downloaded yet.` in gray, 7 px under the section title, until there's a file.

The selection card: `N tracks selected` in the title's font and line, without a glyph; then **Status**, with a row for
each status that holds any of the tracks, in the order Synced (with Needs review), Waiting, Failed, Removed, Kept: the
word as the key, and its 16 px icon, 4 px, and the count as the value. When all of them are Removed, "No longer in any
playlist. Their files are still in the library." follows 6 px below in gray.

**No playlists yet** (first run): empty.

**Nothing selected**:

- In a playlist scope it shows a playlist card with:
  - glyph and name, URL link, track count;
  - a synced/total bar with the counts;
  - how many tracks it shares with other playlists of the same source (shared tracks download once);
  - last sync;
  - `Sync playlist` and `Remove playlist…`.
- In All tracks it shows the library folder, totals and `Open library folder`.

```
┌ inspector ──────────────────────┐   ┌ inspector ──────────────────────┐
│ ▤ All tracks                    │   │ ● WCS                           │
│                                 │   │   YouTube Music playlist        │
│ Library ─────────────────────── │   │                                 │
│ Folder    C:\Users\…\WestieLib  │   │ Playlist ────────────────────── │
│ Tracks    0 tracks · 0 synced   │   │ Link      PLGXm7qiSwxfBEFj0x0K… │
│                                 │   │ Tracks    0                     │
│ [Open library folder]           │   │ Synced    ░░░░░░░░░░░░░░░░░ 0/0 │
│                                 │   │ Last sync Never                 │
│                                 │   │                                 │
│                                 │   │ [⟳ Sync playlist] [Remove pla…] │
└─────────────────────────────────┘   └─────────────────────────────────┘
```

The cards, from the top, in DIPs:

- **Title**: a 16 px glyph (▤ for All tracks, the source's logo for a playlist), 7 px, then the name in semibold at
  125% of the message font, on a 20 px line. A long name ends in an ellipsis.
- **Subtitle** (playlist card only): `YouTube Music playlist` or `Spotify playlist` in gray, 1 px below, lined up with
  the name.
- **Section**, 14 px below: its title (`Library`, `Playlist`) in semibold, followed 8 px on by a `#DADADA` hairline to
  the right edge, then 7 px to the rows.
- **Rows**: a gray key in a 74 px column, 8 px, then the value; rows are 16 px lines, 4 px apart. Values end in an
  ellipsis, except the folder, which loses its middle (`C:\Users\...\WestieLib`).
  - Library: `Folder` is the library folder in effect, `Tracks` reads like the All tracks row (`0 tracks · 0 synced`).
  - Playlist: `Link` is the playlist's ID (the source shows in the title and subtitle above) as a link to its URL (`#0066CC`, underlined while the mouse is over it, the hand
    cursor over its text); a click opens it in the browser. `Tracks` is the count, `Synced` a bar like the row's
    that fills the value's width, 6 px, then `synced/total`. Then, each only when it isn't zero: `Waiting`, `Failed`
    (in red), `To review` (in amber) and `Shared` (`3 tracks are also in other playlists`, counting the playlists of
    the same source). `Last sync` reads like the row without `Synced`: `today 14:03`, `yesterday`, `12 Oct`, or
    `Never`.
- **Buttons**, 14 px below, 6 px apart, wrapping onto a new row 6 px down when they don't fit: `Open library folder`;
  or `⟳ Sync playlist` and `Remove playlist…`. They follow the command rules (see Menu bar and toolbar).

The inspector scrolls vertically when its content is taller than the panel, inside the same padding: 12 px on the
sides and top, 10 px at the bottom. Another card, or the same card for another track or playlist, starts at the top;
a card that only changes (a sync reports progress) keeps its place.

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
| Run     | `Ready · last sync today 14:05` (`Ready` before any sync), or `Syncing 6 of 25` with an embedded 120 px progress bar against the part's right edge, or `⧗ Waiting for ffmpeg` while all that's left of the run waits for tools. So far nothing downloads, so it counts playlists while they are fetched (`Syncing 1 of 2` while the first is), then the tracks it matches (`Syncing 6 of 25`) |
| Tracks  | `100 tracks` (`1 track`) |
| Library | the library folder, `D:\Music\WestieLib` |
| Tools   | `Tools ✓` or `Tools ⚠`, or while the tools are at work, `Tools ◔ installing ffmpeg 35%` with the progress ring: the word, 7 px, a 12 px icon, and then 7 px and what is going on (see below) |
| Version | `v1.2.0` |

```
│ Ready      │ 0 tracks    │ C:\path            │               │ Tools ✓                       │  v0.1.0 │
│ Ready      │ 0 tracks    │ C:\path            │               │ Tools ◔ installing ffmpeg 35% │  v0.1.0 │
```

- The status bar is the native one, 23 px tall, or taller when the system font needs it. It ends in a size grip.
- The parts follow the window's width. Each takes a share of the status bar's width, but never less than its minimum:

  | Part     | Share   | Minimum | Why the minimum                                              |
  |----------|---------|---------|--------------------------------------------------------------|
  | Run      | 22%     | 250 px  | `Syncing 12 of 400` and its 120 px bar                       |
  | Tracks   | 10%     | 100 px  | `12,345 tracks`                                              |
  | Library  | 36%     | 150 px  | a drive and a folder or two; the middle of a path gives way  |
  | (gap)    | the rest | 0      |                                                              |
  | Tools    | –       | its widest text | `Tools ◔ updating yt-dlp 100%`, so the part never changes width   |
  | Version  | –       | 80 px   | `v1.2.0`                                                     |

  Tools is measured in the status bar's font against every text it can show, with the percentage at `100%`: about
  200 px at 100% scaling. So at 1280 px (the mockup) Run, Tracks and Library are 278, 126 and 455 px, with a gap of
  about 125 px; at the 1100 px minimum they're 250, 108 and 390 px; maximized on a 1920 px screen, 418, 190 and 685 px.
- Run, Tracks and Library sit on the left. Tools and Version sit against the right edge, Version last, with its text
  centered between its separator and the grip, or the window edge when maximized. No separator follows it. The gap
  between Library and the right-hand parts stays empty. When the window is too narrow for all of them, Library gives
  way.
- Texts keep 8 px from the edges of their part, and end in an ellipsis when they don't fit. The library folder
  loses its middle instead (`C:\Users\...\WestieLib`), so its last folder stays readable.

**Tools.** The part shows the first that applies:

| Text                              | Icon                           | Shown when                                    |
|-----------------------------------|--------------------------------|-----------------------------------------------|
| `Tools ◔ installing ffmpeg 35%`   | progress ring, filled to 35%   | a missing tool downloads, from 0% while it connects |
| `Tools ◔ updating yt-dlp 12%`     | progress ring, filled to 12%   | a newer release of an installed tool downloads |
| `Tools ● installing ffmpeg`       | full progress ring             | the download is checked, unpacked and put in place (`updating yt-dlp` for an update) |
| `Tools ○ installing ffmpeg`       | empty progress ring            | the release of a missing tool is looked up    |
| `Tools ○ checking for updates`    | empty progress ring            | the releases of installed tools are looked up |
| `Tools ○ checking`                | empty progress ring            | a run is between those steps, as when it starts |
| `Tools ⚠`                         | amber warning                  | a tool is missing, or its last install failed |
| `Tools ✓`                         | green check                    | every tool is installed                       |

The progress ring is the track table's (see Track table): a gray circle with a blue arc that fills clockwise from 12
o'clock, in 24 steps. Its tooltip has a line per tool: `yt-dlp 2026.10.21`, `ffmpeg n9.0`, `yt-dlp: not installed`,
or `ffmpeg: <error>` after a failed install or update.

## Dialogs

Modal dialogs follow native behavior: the main window turns inactive, and nothing is dimmed.

- **Add playlist**:
  - "Paste a link to a public YouTube Music or Spotify playlist."
  - A URL edit, wide enough to show a whole shared link (`https://open.spotify.com/playlist/<id>?si=<16 characters>`,
    or a YouTube Music link with a 34-character list id) without scrolling, with a live detection line under it.
    Spaces and line breaks around a pasted link don't count. The line shows the first that applies:

    | Detection line                                  | Icon      | Shown when                                        | Add |
    |-------------------------------------------------|-----------|---------------------------------------------------|-----|
    | (empty)                                         | –         | the edit is empty                                 | off |
    | `Not a supported playlist link`, in gray        | –         | the text isn't a playlist link WestieLib takes    | off |
    | `Waiting for required tools to be installed…`, in gray | ⧗  | a YouTube Music link, while yt-dlp isn't installed | off |
    | `Looking up the YouTube Music playlist…`, in gray | spinner | the look-up runs                                  | off |
    | `WCS` `YouTube Music playlist`                  | logo      | the look-up found the playlist, named `WCS`       | on  |
    | `This playlist doesn't exist or is private.`    | ⚠         | the look-up found no public playlist              | off |
    | `Couldn't look it up: <reason>`                 | ⚠         | the look-up failed otherwise, say without network | off |

    - The spinner shows that the look-up is going on. It is the progress ring's gray circle (see Track table) with a
      quarter of its blue arc, which turns clockwise smoothly, once a second. When Windows' animations are off
      (`SPI_GETCLIENTAREAANIMATION`), it stands still. The source's logo takes its place once the playlist is found.
    - A found playlist's line is its name in the text color, then, 6 px on, `YouTube Music playlist` or
      `Spotify playlist` in gray. A long name ends in an ellipsis, so the source's name stays.
    - **Already added.** A link to a playlist that is in the list already shows the line of a found playlist, with
      the name from the list, and `Add` reads `Already added` and stays disabled. A playlist stored without a name (by
      an earlier version) is looked up as usual for its line. `Add` is wide enough for `Already added`.
    - Two links name the same playlist when they have the same playlist id, so a shared link's `?si=…` makes no
      difference, and changing only that part doesn't look the playlist up again. The links WestieLib takes are in
      `arch.md` (Playlist links).
    - The look-up starts 300 ms after the link last changed, so a link typed by hand isn't looked up at every key. A
      new link cancels the look-up of the one before.
    - Looking up a YouTube Music playlist needs yt-dlp; a Spotify playlist needs no tool. While yt-dlp isn't installed
      the line waits, and the look-up starts by itself once yt-dlp is (see Vocabulary › Waiting for tools).
    - A long reason ends in an ellipsis.
  - A `Sync now` check box (on by default). With it, `Add` also syncs the new playlist; during a sync it joins the
    sync's queue.
  - `Add` (default, enabled only once the look-up found a playlist that isn't in the list yet) and `Cancel`. `Cancel`
    also cancels the look-up.
  - `Add` stores the playlist with the name the look-up found. The welcome screen gives way to the main panel, and the
    playlist shows in the left panel.
- **Choose a video**. Opened by `Choose video…` in the inspector, the Track menu or Edit track. One dialog
  covers both the case with several candidates and the case where nothing was found:
  - At the top: the source track (glyph, title, artists, album, length) and either "Current match: <title>,
    58% confidence" or "The Matcher found no usable video."
  - `Search YouTube Music:` an edit pre-filled with the Matcher's query, a `Songs and videos` / `Songs` /
    `Videos` drop-down, and `Search`. Searching needs yt-dlp: without it, `Search` is disabled and the results list
    reads "Search needs yt-dlp, which isn't installed yet." Pasting a link still works.
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
    - One row per download: `yt-dlp` and `ffmpeg` (ffmpeg brings ffprobe along). The release is empty for a tool
      that isn't installed.
    - The status is an icon and a word, as in the Tools part: ✓ `Up to date` (checked since the app started) or
      `Installed`; ⟳ `Checking…`, `Downloading 45%` or `Installing…`; ⚠ `Not installed` or `Failed: <error>`.
    - The list follows the tools while the dialog is open.
    - `Check for updates` starts at once, without waiting for `OK`, and updates installed tools even when the check
      box is off. It is disabled as in the Tools menu.
    - "Last checked" reads like a playlist's last sync: `Last checked today 14:03`, `Last checked yesterday`,
      `Last checked 12 Oct` (`12 Oct 2025` in another year), or `Never checked`.
  - **Maintenance**: `Rebuild library from files…`.
  - The footer shows the version on the left, and `OK` / `Cancel`.
  - So far the dialog has the Library and Tools groups; the other groups come with their features. A folder chosen
    with `Change…` and the check box take effect on `OK`.
- **About** is a TaskDialog with the app icon, `WestieLib` as the main instruction, `Version 1.2.0` (the build's
  version) and OK.
- **Confirmations** use a TaskDialog:
  - Remove playlist: title `Remove playlist`, main instruction `Remove "<name>"?`, content "Synced tracks will not be
    deleted from your library." Buttons `Remove` and `Cancel`, with Cancel the default. Tracks left in no playlist
    become Removed, and the infobar offers to review them afterwards.
  - Move to Recycle Bin says how many files.
  - Rebuild says it replaces track entries.
- **First run** has no playlists, so the main panel shows its welcome screen (see Main panel › First run). Tools install
  in the meantime, and the status bar shows their progress.

## Command and event mapping

| Core API | UI |
|----------|----|
| `addPlaylist` | toolbar, left panel button, File menu, first-run button → Add playlist dialog |
| `lookUpPlaylist` / `PlaylistLookedUp` | Add playlist dialog's detection line |
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
| Frame | HMENU + accelerator table; the menu bar painted through the undocumented `WM_UAHDRAWMENU` / `WM_UAHDRAWMENUITEM` with the `Menu` theme; three custom splitters |
| Toolbar | `TOOLBARCLASSNAME` with `TBSTYLE_FLAT \| TBSTYLE_LIST` and an image list; `BTNS_DROPDOWN` + `TBSTYLE_EX_DRAWDDARROWS` for Sync all |
| Infobar | custom-painted child window holding push buttons |
| Status bar | `STATUSCLASSNAME` with owner-drawn parts, an embedded `PROGRESS_CLASS` and a `TOOLTIPS_CLASS` tool over the Tools part |
| Left panel | All tracks is a custom-painted row window above the list. The playlist list is a custom-painted window with its own vertical scroll bar; rows, bars and icons are drawn in its `WM_PAINT`. It isn't a `LISTBOX`, because list box items are at most 255 px tall, which a playlist row passes above ~260% scaling. `No playlists yet.` is a label, and `+ Add playlist` a push button with an icon; both are children of the list, placed after the last row, so they scroll with it. Rows hot-track with `WM_MOUSEMOVE` + `TrackMouseEvent` (`WM_MOUSELEAVE`); `WM_LBUTTONDOWN` presses the row it hits, takes the mouse capture and the focus, and `WM_LBUTTONUP` over the same row selects it. The list's `WM_KEYDOWN` moves the selection. `WM_CONTEXTMENU` selects the row and shows the context menu with `TrackPopupMenu` (`TPM_RETURNCMD`). The per-row sync control is drawn in the same pass, right after the measured line-5 text (`GetTextExtentPoint32`), with the hover colors `#E5F3FF` and a `#CCE8FF` frame (`#99D1FF` on a selected row) while the mouse is over it. It is hit-tested in `WM_LBUTTONDOWN`, takes the mouse capture until `WM_LBUTTONUP`, hot-tracked with `WM_MOUSEMOVE` + `TrackMouseEvent`, and has a tooltip-control tool per row rectangle while it can be clicked. The spinning sync icon is 12 icon resources (30° steps) advanced by a ~100 ms `WM_TIMER` that invalidates only the icon rectangles. The timer runs only while a sync is active. When `SPI_GETCLIENTAREAANIMATION` is off, the icon stays still. The inspector's `Downloading` icon uses the same frames |
| First run | a panel that paints the icon, with labels, an `EDIT` (`ES_AUTOHSCROLL`; the hint follows `EN_CHANGE`) and push buttons; the picker is `IFileOpenDialog` with `FOS_PICKFOLDERS` |
| Filter bar | `EDIT` with `EM_SETCUEBANNER`, a `CBS_DROPDOWNLIST` combo box, a push button |
| Status filter | `WC_TABCONTROL` with `TCS_BUTTONS \| TCS_FLATBUTTONS`; labels rewritten with counts. The control's own buttons are classic 3D ones on a gray band, so its subclass paints the buttons, the hover and the separators in `WM_PAINT`, and the control keeps the clicks, keys and selection |
| Track table, log | `WC_LISTVIEW` with `LVS_REPORT \| LVS_OWNERDATA`, `LVS_EX_FULLROWSELECT \| LVS_EX_DOUBLEBUFFER \| LVS_EX_HEADERDRAGDROP`; `NM_CUSTOMDRAW` for the Status icon and colored text; `SetWindowTheme(L"Explorer")`, which also draws faint lines under the column dividers. The header's `NM_CUSTOMDRAW` post-paint draws the rule under the titles, which Explorer's design leaves out. The progress ring comes from a 24-step image list. The Status tooltip uses `LVS_EX_INFOTIP` + `LVN_GETINFOTIP`. The # column is inserted when a playlist is selected and removed for All tracks |
| Inspector | a child window with its own vertical scroll bar, holding one card at a time. A card is a panel of labels, buttons and small self-painted windows: the title, the section titles with their rules, the synced bar and the link. The link isn't `WC_LINK`, which can't end in an ellipsis and takes its background from its parent; it hot-tracks like the playlist rows. Covers decoded with WIC into an HBITMAP |
| Dialogs | `DIALOGEX` resources, `UPDOWN_CLASS`, an `SS_OWNERDRAW` static for each detection line (icon and text), a `WM_TIMER` that delays the playlist look-up and another that redraws its spinner about 60 times a second (drawn with GDI+, its angle from the time since it started), `BS_AUTORADIOBUTTON`, `IFileOpenDialog` (`FOS_PICKFOLDERS` for the library, image filter for covers), `TaskDialogIndirect`. The Choose a video results are a report ListView, and its empty text comes from `LVN_GETEMPTYMARKUP`. Edit track's cover is an `SS_BITMAP` static, and Paste reads `CF_DIB` |
| Global | ComCtl32 v6 manifest, Per-Monitor V2 DPI awareness, message font from `SPI_GETNONCLIENTMETRICS`. Every `EDIT` is subclassed for the word keys: Ctrl+Backspace arrives as `WM_CHAR` `0x7F`, which the control would type as a box. The subclass also keeps the undo history (`EditHistory`), taking Ctrl+Z and Ctrl+Y (`WM_CHAR` `0x1A` and `0x19`) and swallowing the edit's own one-step undo: `WM_UNDO`, `EM_UNDO`, `EM_CANUNDO` and Alt+Backspace. It swallows `WM_CONTEXTMENU` too |

The UI uses the Windows 10 palette:

| Role | Color |
|------|-------|
| window | `#FFFFFF` |
| face | `#F0F0F0` |
| selection | `#CCE8FF`, border `#99D1FF` |
| hover | `#E5F3FF` |
| link | `#0066CC` |
| pressed button | `#CCE4F7`, border `#005499` |
| accent | `#0078D7` |
| gray text | `#6D6D6D` |
| progress | `#06B025` |
| infobar | `#FFF4CE` |

Nothing in the UI uses rounded cards, shadows or gradients.

**Brand logos.** The YouTube Music, Spotify and YouTube icons ship as icon resources made from the official
brand assets; the mockup's versions are drawn approximations.
