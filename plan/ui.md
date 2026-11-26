# WestieLib UI

The UI is a single native Win32 main window with a Windows 10 light look: Segoe UI, system colors, Explorer-style
list views. It has three panels: playlists on the left, tracks in the middle, and an inspector on the right.
Under the toolbar, a window-wide infobar appears when something needs attention. Above the status bar, a
window-wide log pane can be shown.

The static screens in `ui-mockup.html` are the visual reference for this spec.

## Window layout

```
┌ WestieLib ──────────────────────────────────────────────────────────────── ─ □ ✕ ┐
│ File  Update  Track  View  Tools  Help                                           │ menu bar
│ [+ Add playlist] [⟳ Update all ▾] [■ Cancel]  [⚙ Settings]                       │ toolbar
│ ⚠ 5 tracks are no longer in any playlist.                  [Review…] [✕]         │ infobar (when needed)
├────────────────────────┬────────────────────────────────────────┬────────────────┤
│ ▤ All tracks           │ [🔍 Search title, artist, album ] Source [All ▾]      │ filter bar
│   412, 398 downloaded  │ All 412│Downloaded 398│Waiting 6│Needs review 3│Fail… │ status filter
├────────────────────────┤ Showing 214 of 412 in WCS                       │                │
│ ▶ WCS                  ├────────────────────────────────────────┤ [ cover art ]  │
│   214 tracks           │ # Status  Title    Artists  Album    … │ Title          │
│   ███████████░ 198/214 │ 1 ✓       …        …        Freudian   │ Artists        │
│   14 waiting, 2 failed │ 2 ⚠       …                            │ Album (2021)   │
│   ✓ Updated 14:03 ↻    │ 3 ◔                                    │ ── Status ──   │
│ ● Her playlist         │                                        │ ── Source ──   │
│   98 tracks            │                                        │ ── Audio ──    │
│   ██████░░░░░░  51/98  │                                        │ ── File ──     │
│   Matching 6 of 47 ⟳   │                                        │ [actions]      │
│ ● Slow & bluesy        │                                        │                │
│   ⚠ Couldn't sync ↻    │                                        │                │
│ [+ Add playlist]       │                                        │                │
├────────────────────────┴────────────────────────────────────────┴────────────────┤
│ log pane (hidden by default, View › Log, Ctrl+L)                                 │
├──────────────────────────────────────────────────────────────────────────────────┤
│ Matching 12 of 40 ▓▓▓░░░ │ 412 tracks │ D:\Music\WestieLib │    │ Tools ✓ │ v1.2.0 │ status bar
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
  shows, the log's level and scope, the track table's column widths and the Choose a match dialog's size are saved
  under `HKCU\Software\WestieLib\Window` when the window closes, and restored on the next start. A minimized window comes back restored, or maximized if it was maximized before.
- The inspector is always shown. View › Log toggles the log pane.
- Minimum window size is about 1100×680. The mockup draws the window at 1280×800, with a 250 px left panel and
  a 290 px inspector.

## Vocabulary

The UI uses its own words for the core statuses. The whole run is an *update*: for each playlist it **syncs** (reads
the playlist's data), then **matches** (Spotify only: searches YouTube Music for each track), then **downloads**. A
downloaded track is **Downloaded**.

| Core status / flag | UI word                              | Icon                               |
|--------------------|--------------------------------------|------------------------------------|
| `downloaded`       | Downloaded                           | green circle with check            |
| `pending`          | Waiting                              | hollow gray circle                 |
| (downloading)      | Waiting, shown as `Downloading 62%`  | blue progress ring that fills      |
| (needs a tool)     | Waiting, shown as `Waiting for ffmpeg` | gray hourglass, still            |
| `failed`           | Failed                               | red circle with cross              |
| (unconfirmed low-confidence match) | Needs review         | amber warning triangle             |
| `orphaned`         | Removed                              | orange circle with slash           |
| `kept`             | Kept                                 | gray pin                           |
| (a playlist's later entry of a track) | Duplicate         | two overlapping gray squares, nearly square-cornered |

In the Status column, Needs review wins over Downloaded and Waiting and shows ⚠, because review is the state the user
can act on. A track that needs review doesn't download until its match is confirmed or another one chosen
(`arch.md`, YouTube Music id resolution), so it is a Waiting one. Duplicate wins over all of them, but only in the Status column and the inspector's status line: a
duplicate counts under its track's status everywhere else.

**Needs review.** A Spotify track needs review when its YouTube Music id comes from a match the Matcher wasn't sure of (`arch.md`,
Matcher › Verdict: a Review, with the doubts that made it one) and that the user hasn't confirmed. Until then the match
stays apart: it doesn't merge into a track that already has that id.

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
| Download a track: audio, thumbnail cover  | yt-dlp, ffmpeg, deno |
| Choose a match: search                    | –               |

A track that waits shows the hourglass in the Status column, with `Waiting for ffmpeg` (or `Waiting for yt-dlp and
ffmpeg`) as its tooltip and in the inspector. A playlist whose fetch waits shows it on line 5 (see Left panel). The
Add playlist dialog shows it on its detection line (see Dialogs).

**Tracks, sources and entries.** A *track* is one file in the library. A track has one or more *sources*, which
are YouTube Music or Spotify catalog entries. Sources with the same YouTube Music id are merged into one track (see `arch.md`,
Merging): the same song in a YouTube Music and a Spotify playlist, or one recording a Spotify playlist lists under
several ids (single, album, compilation). All tracks shows a row per track. A playlist shows a row per *entry*: one
source it lists. So a merge never takes a row out of a playlist: when a playlist lists several sources of one track,
the first is the original and the later ones are *duplicates*, which download nothing of their own and share its
file. Entries in different playlists are never duplicates of each other. A track with a YouTube Music source doesn't
need review, because the playlist owner chose its id, unless YouTube Music no longer plays that id here: then the track
is matched like a Spotify one, and its match can need review (`arch.md`, Unavailable ids).

## Menu bar and toolbar

Each menu lists its items top to bottom; ─ is a separator. Shortcuts are right-aligned in the menu.

| Menu  | Items |
|-------|-------|
| File  | Add playlist… (Ctrl+N), Remove playlist…, ─, Open library folder, ─, Exit (Ctrl+Q) |
| Update | Update all (F5), Update selected playlist, ─, Cancel |
| Track | Confirm match, Choose match…, Edit track… (Enter), ─, Retry, Keep, Delete file…, ─, Show in folder, Show in log, Open on YouTube Music, Open on Spotify |
| View  | Log (Ctrl+L), ─, Columns… |
| Tools | Settings… (Ctrl+Alt+S), Check for updates, ─, Rebuild library from files…, Locate library folder…, ─, Open log folder |
| Help  | About WestieLib |

Access keys: the menu bar has **F**ile, **U**pdate, **T**rack, **V**iew, T**o**ols and **H**elp (Track and Tools can't
share T). The items, in order: File **A**dd, **R**emove, **O**pen, E**x**it; Update **A**ll, **S**elected, **C**ancel;
Track **C**onfirm, C**h**oose, **E**dit, **R**etry, **K**eep, **D**elete, **S**how in folder, Show in **l**og,
**Y**ouTube Music, S**p**otify;
View **L**og, **C**olumns; Tools **S**ettings, **U**pdates, **R**ebuild, **F**older, **L**og folder; Help **A**bout.

The menu bar is white and runs straight into the toolbar, without the gray line Windows draws under a menu bar. Its
items look as usual: the same font, hover and open highlights, and gray text while the window is inactive.

The toolbar has `+ Add playlist`, `⟳ Update all ▾`, `■ Cancel` and `⚙ Settings`.

- It is a white strip, 34 px tall including a 1 px `#D9D9D9` rule along its bottom, and as wide as the window.
- The buttons are flat, 26 px tall and centered in the strip, starting 4 px from its left edge. Each has 8 px on
  either side, a 16 px icon, 5 px, then its text. Buttons sit 1 px apart. A separator between Cancel and Settings
  takes 11 px: a 1 px `#D5D5D5` line, 20 px tall and centered, with 5 px on either side.
- A disabled button shows its icon in grayscale at 45% opacity, and its text in `#A0A0A0`, antialiased like all other
  text.
- The strip grows when the system font needs taller buttons.

- `Update all ▾` is a split button. Its dropdown lists "Update selected playlist" and then every playlist by name.
- The menu bar, the toolbar and the shortcuts share one set of rules, so a command is enabled everywhere or nowhere:
  - `Cancel` is enabled only while an update runs. `Update all` is enabled when there is at least one playlist and no
    update runs. The tools don't matter: work that needs a missing tool waits (see Vocabulary › Waiting for tools).
  - `Remove playlist…` and `Update selected playlist` need a selected playlist, and `Update selected playlist` also
    needs no update running.
  - Track items need a selected track. `Confirm match` needs exactly one selected row, and that it needs review.
    `Choose match…` needs exactly one selected row whose entry the Matcher matches: a Spotify track, or a YouTube Music
    entry whose id is unavailable (`arch.md`, Unavailable ids). `Retry` needs a selected row whose
    track failed, and retries every such track the selection holds; it is enabled during an update too, whose queues
    the tracks join. `Show in folder` needs exactly one selected row whose track has a file. `Show in log` needs a
    selected row. `Open on YouTube Music` needs exactly one selected row whose track has a YouTube Music id, the one
    the card's Audio section shows, and opens its `music.youtube.com/watch` page. `Open on Spotify` needs exactly one
    selected row whose track has a Spotify source, and opens its `open.spotify.com/track` page: the row's own entry when
    that is a Spotify one, else the track's first Spotify source.
  - **Show in log** shows the log pane when it is hidden and sets its scope to `Selected track`, so it lists the lines
    of the selected tracks. The level stays as it is.
  - So far `Edit track…` stays disabled; it is enabled with its feature.
  - `Check for updates` is disabled while tools are checked, installed or updated, and while an update runs.
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
the infobar's `Review…` empties the search, starts a new history. Esc and the search field's ✕ are changes like typing,
so Ctrl+Z brings the search back. The shortcuts are the only way in: Alt+Backspace does nothing.

Text fields have no context menu; a right-click or the Menu key does nothing.

## Infobar

A yellow strip under the toolbar, in the Office/Edge style. It holds an icon, a message, action buttons and ✕.
Only one message shows at a time, in this priority order:

| Message                                                 | Actions                                                 |
|---------------------------------------------------------|---------------------------------------------------------|
| "5 tracks are no longer in any playlist."               | `Review…` selects All tracks and the Removed status tab |
| `Couldn't fetch "Slow & bluesy". It may be private or deleted.` | `Details` opens the log                         |
| "The library folder D:\Music\WestieLib is missing."     | `Locate…` relocates it                                  |
| "WestieLib 1.3.0 is available." (when self-update can't install) | `Download page`                                |

✕ hides the current message until its cause changes. A dismissed fetch failure still shows ⚠ on its playlist
row.

So far it shows removed tracks and fetch failures. Removed tracks: the count of Removed tracks in the library (`1 track
is no longer in any playlist.` for one), as long as there are any; dismissed, it shows again once a track that wasn't
Removed then becomes Removed. `Review…` selects All tracks and the Removed tab. Fetch failures, one at a time: the
first playlist, in the order they were added, whose last fetch failed and wasn't dismissed. A later failure of a
dismissed playlist shows again. `Details` shows the log pane, where the failure is an Error line.

```
│ ⚠  Couldn't fetch "Slow & bluesy". It may be private or deleted.                         [Details]  ✕ │
```

The strip is `#FFF4CE` with a 1 px `#EADB9F` rule along its bottom. From the left: 10 px, the 16 px ⚠, 8 px, the
message (an ellipsis when it doesn't fit), 8 px, `Details` (a compact button), 8 px, and ✕ in a 24 px box, 6 px from
the right edge. A click on the box dismisses the message.

## Left panel

The left panel has the All tracks row at the top, a separator under it, and then the list of playlists. All tracks
stays where it is; only the playlist list scrolls.

**All tracks** (glyph ▤): `All tracks`, then `100 tracks, 92 downloaded`. It is selected while the main panel shows the
whole library, which is always the case until there are playlists.

With no playlists, the list reads `No playlists yet.` in gray.

**Playlist rows** have variable height:

| Line | Content |
|------|---------|
| 1 | Source logo (the YouTube Music or Spotify icon, 14 px) and the name in semibold. The name comes from the look-up in Add playlist and is refreshed by every sync. A playlist stored without a name (by an earlier version) reads `YouTube Music playlist` or `Spotify playlist` until its first sync |
| 2 | `55 tracks` |
| 3 | downloaded/total bar, then `51/55` |
| 4 | non-zero counts only: `3 waiting, 1 failed, 2 to review` in gray / red / amber. Left out when all are zero |
| 5 | status icon, last-update text, update control (see below); during an update, its phase instead |

**Line 5** reads like `✓ Updated today 14:03 ↻`:

- **Status icon.** It starts the line, flush with the progress bar above, and the text follows it. It is the
  first rule that applies, top to bottom:

  | Icon | Status            | Shown when                                          |
  |------|-------------------|-----------------------------------------------------|
  | ⚠    | sync failed       | the last sync failed                                |
  | ✕    | failures          | some tracks failed                                  |
  | ⧗    | waiting for tools | its sync, or some of its tracks, wait for a tool    |
  | ○    | not done          | some tracks are waiting, or it was never updated    |
  | ✓    | done              | none of the above                                   |

  While it waits for tools, the last-update text reads `Waiting for yt-dlp`.

- **Last-update text.** One of `Updated today 14:03`, `Updated yesterday`, `Updated 12 Oct`, `Never updated`, or
  `Couldn't sync, updated 9 Oct` (with `Couldn't sync` in amber). The time is when the playlist last synced. It ends in an ellipsis rather than pushing
  the control out of the row.
- **Update control.** It sits directly after the text: a 20 px flat button with a gray ↻ glyph and no frame.
  - On hover it gets the light-blue hot frame and the tooltip "Update <playlist name>". Clicking it updates that
    playlist only.
- **Phase.** While the playlist is in an update, the line shows its phase instead, and the status icon steps aside
  because the update supersedes it. The first that applies:

  | Phase       | Shown when                                                  | Text                  | Control               |
  |-------------|-------------------------------------------------------------|-----------------------|-----------------------|
  | queued      | it is in the run, and its sync hasn't started               | `Queued`              | ⟳, still and faded    |
  | syncing     | its data is read                                            | `Syncing…`            | ⟳, spinning           |
  |             | its sync waits for a tool                                   | ⧗ `Waiting for yt-dlp` | ⟳, still and faded   |
  | matching    | Spotify only: the run's match queue holds tracks of it       | `Matching 12 of 340`  | ⟳, spinning while the track the update last started matching is one of its tracks, so it keeps turning between two of them; still and faded otherwise |
  | downloading | its tracks download                                         | `Downloading 6 of 22` | ⟳, spinning           |
  |             | its downloads wait for a tool                               | ⧗ `Waiting for ffmpeg` | ⟳, still and faded   |

  The spinning ⟳ is blue and turns clockwise at about one turn per 1.2 s. A playlist whose phases are over shows its
  done line (above) while the update goes on with others.
  - The matching counts are the playlist's tracks the update queued to match, and how many of them have ended:
    matched, merged, failed, or skipped because they no longer needed a match. While one of its tracks is matched, that
    one counts too, as in the status bar: the first track matched reads `Matching 1 of 340`. A track that two playlists list counts
    in both. A search YouTube Music refused doesn't end its track; the track waits in the queue (`arch.md`,
    SyncEngine).
  - An update matches only after every playlist in it has synced, so a synced Spotify playlist reads
    `Matching 0 of 340` with the still ⟳ until then.
  - The downloading counts work the same way: the playlist's tracks the update queued to download, and how many of them
    have ended (downloaded, failed, merged or skipped), plus those downloading right now. A track two playlists list
    counts in both. The ⟳ spins for as long as the phase lasts, also in the pause between two tracks.

The playlists are listed in the order they were added. A playlist that was just added reads `0 tracks`, an empty bar
with `0/0`, and `○ Never updated ↻`.

**So far** an update syncs playlists, YouTube Music and Spotify alike, matches the Spotify tracks, and then
downloads the tracks that have a YouTube Music id and don't need review. It handles the tracks of the playlists it
updates only: `Update selected playlist` leaves the tracks that wait in other playlists alone, and their rows don't
show a phase. A track shows as Waiting until its download ends, then
as Downloaded, or as Failed when its match or download fails. Meanwhile:

- a YouTube Music playlist whose sync waits for yt-dlp shows ⧗ and `Waiting for yt-dlp`, with the still, faded ⟳ of
  a queued one. A Spotify sync needs no tool, so it never waits;
- a Spotify playlist's tracks show 100 at a time while it syncs: the row's counts, the table and the cards
  update with every page. A sync that fails or is cancelled halfway keeps the tracks it read, beside the ones the
  playlist had; the next sync that succeeds settles the list;
- a sync that succeeds sets the time of the last update, so once its matching is over a playlist whose tracks all
  wait reads `○ Updated today 14:03 ↻`;
- each match and each download updates the table, the cards and the rows' counts as it ends. A matched track that merges into one
  with the same YouTube Music id leaves the table; when it was selected, the selection is gone with it.

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
- While the list has the focus, Del is Remove playlist… for the selected playlist. With All tracks selected it does
  nothing. Shift+Del, Ctrl+Del and Alt+Del do nothing either.
- Adding a playlist selects it. When the selected playlist goes away, All tracks is selected.
- Every start begins on All tracks; the selection isn't saved.

**Context menu.** A playlist row's context menu has **U**pdate, **O**pen in browser, **C**opy link, ─, **R**emove
playlist… (Del). The shortcut is right-aligned, as in the menu bar.

- A right-click selects the row under the mouse first, and gives the list the focus. A right-click between or below
  the rows, or on All tracks, shows no menu.
- Shift+F10 and the menu key open it for the selected playlist, at the row's bottom-left corner, while the list has
  the focus. With All tracks selected they show nothing.
- Its items follow the command rules: they need a selected playlist, and Update also needs no update running. Update
  does what `Update selected playlist` does.
- **Open in browser** opens the playlist's URL in the default browser, as the playlist card's link does. **Copy link**
  puts the URL on the clipboard; when another program holds the clipboard, it beeps instead.

**Remove playlist…** (File menu, context menu, playlist card) asks first (see Dialogs › Confirmations). After a
removal All tracks is selected; removing the last playlist brings back the welcome screen and the inspector's
placeholder.

**Open library folder** (File menu, Library card) opens the library folder in Explorer, or the nearest folder above it
that exists while the first update hasn't created it yet. When even the drive is missing it does nothing. When Windows
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

- starts as `Music\WestieLib` in the user's profile, the *default folder*. It doesn't have to exist; the first update
  creates it.
- The hint line under the field says what the text in the field names, and changes as the user types:

  | The text | Hint |
  |----------|------|
  | isn't a full path (`C:\…` or `\\server\share\…`), or has a character a folder name can't have | "Enter a full folder path, such as `<default folder>`." in red |
  | is on a drive letter this PC doesn't have | "That drive isn't on this PC." in red |
  | has a folder name that ends in a space or a period, which Windows drops | "A folder name can't end in a space or a period." in red |
  | has a folder name Windows keeps for devices (`CON`, `NUL`, `COM1`, … with or without an extension) | "CON, NUL, COM1 and the like are names Windows keeps for devices." in red |
  | is a file | "That's a file, not a folder." in red |
  | is a folder that doesn't exist yet | "This folder doesn't exist yet. The first update creates it." in gray |
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
  Esc empties the field. Ctrl+F (not in a menu)
  puts the focus in it and selects its text. A 16 px magnifier sits inside the field, 5 px from its left edge, and the
  text starts 5 px after it.
- While the search field holds text, a ✕ sits inside it against its right edge: the close button's icon at 10 px, in
  an 18 px box, 2 px from the field's right border and centered top to bottom, with the close button's hover and
  pressed colors. The text stops 4 px before the box, and the field keeps that room while the ✕ is hidden, so the text
  doesn't move when it comes. A click on it empties the field, as Esc does, and puts the focus in the field.
- **Source**: the label `Source`, its text level with the search field's, then a drop-down list with All, YouTube
  Music and Spotify.

The bar has 8 px of padding at the top and sides and 4 px under it, with 8 px between its parts. The search field is
300 px wide, and narrows when the panel is too narrow for it; the drop-down is 128 px wide.

### Status filter

A strip of flat buttons with counts. Exactly one button is selected. The counts take the scope, the search and
the source into account, so each button shows how many rows it would list:

`All │ Downloaded │ Waiting │ Needs review │ Failed │ Duplicates │ Removed │ Kept`

- Each button reads its word and its count, as in `Needs review 3`. The buttons are 22 px tall with 8 px at the
  sides of the text. The strip has 4 px above it, 8 px at the sides and 2 px under it. The selected button has the
  pressed button colors, a button under the mouse the hover color (border `#CCE8FF`), and a 1×14 px `#C9C9C9` line
  separates the buttons.
- `Removed` and `Kept` appear only when their count is not zero, or while they are selected.
- `Duplicates` lists the playlist's duplicates. It shows in every playlist scope, also at 0, and never in All tracks,
  which has none.
- All = Downloaded + Waiting + Failed + Removed + Kept.
- Needs review is a subset of Downloaded and Waiting: a row that needs review also counts under its track's status. So
  is Duplicates of All: a duplicate also counts under its track's status.

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
| Status    | on      | status icon only (56 px). A downloading row shows a blue progress ring instead (the status bar's ring at 16 px, filled as far as yt-dlp's percentage), a row whose download waits for a tool the gray hourglass, and a duplicate the duplicate icon, gray, or green once its track is downloaded. Hovering the cell shows the word as a tooltip (`Downloaded`, `Needs review`, `Downloading 62%`, `Waiting for ffmpeg`, …); a duplicate's reads `Duplicate of track #3 (<its track's word>)`, with the original's position |
| Title     | on      | |
| Artists   | on      | joined with `, ` |
| Album     | on      | Spotify tracks have it from the fetch. Empty for waiting YouTube Music tracks, whose album comes from the info JSON at download time, unless a Spotify source merged in has one. Title, Artists, Album and Time show the track's effective metadata (`arch.md`), so a duplicate reads like its original |
| Playlists | on      | names of every playlist that lists one of the track's sources, joined with `, `, ending in an ellipsis when cut |
| Time      | on      | m:ss |
| Source    | off     | logo of the row's source: the entry's, or in All tracks the track's first entry's |
| Match     | off     | confidence %, `Override` or `Direct` |
| File      | off     | file name |

Rows are 22 px tall. A 1 px rule (`#E5E5E5`) runs under the column titles, across the table's width. The default widths
are # 34, Status 56, Title 204, Artists 156, Album 132, Playlists 114 and Time 48 px; # and Time are right-aligned. A
column the user resizes keeps its width in every scope and across restarts; # keeps its width while All tracks hides it.

Default sort is Title in All tracks and # in a playlist scope. A click on a header sorts by that column, ascending, and
a second click reverses it, however quickly it follows; the header shows the arrow. The sort is stable: rows that tie on
the column keep the order they had before the click, so sorting by Album after Artists sorts each album by artist. A
reverse turns the whole list round, ties included. Rows that tie on every column the user sorted by go by the scope's
default sort, then track key. The order holds while the rows are filtered or refreshed. Another scope starts at its
default sort, and forgets the earlier clicks.

**Double-click.** A double-click on a row that needs a match is `Choose match…` for it: the row needs review, or its
track failed because no match was found or the search failed. On any other row it does nothing yet; it will be
`Edit track…` once that exists. A double-click between or below the rows does nothing.

**Context menu.** A row's context menu is the Track menu, with the same items, access keys and rules.

- A right-click on a row that isn't selected selects it alone first; on a selected row it keeps the selection, so the
  menu acts on every selected row. It gives the table the focus. A right-click between or below the rows shows no menu.
- Shift+F10 and the menu key open it while the table has the focus, at the bottom-left corner of the focused row, or
  of the first selected row when the focused one isn't selected, which scrolls into view first. With nothing selected
  they show nothing.
- An item chosen does what the menu bar's does.

Ctrl+A selects every row, as in Explorer; so does it in the log pane's list.

The selection stays with its tracks while the rows are sorted, searched, filtered by source or refreshed by an update; a
track that is filtered out loses it. Selecting another status tab or another scope (All tracks or a playlist) clears it.

All tracks shows a row per track, for its first entry: in the left panel's playlist order, then by position. A
playlist scope shows a row per entry: each source the playlist lists, at its first position (a source it lists twice
shows once). A row whose track the playlist lists earlier through another source is a duplicate (see Vocabulary).
The status tabs, the result line, the playlist rows' counts and the selection card count rows, a duplicate under its
track's status; the library's counts count tracks.

A refresh, such as each synced page or match of an update, keeps the table where it is scrolled. Another scope, status
tab, search or source starts it at the top.

During an update, the matches and downloads refresh the rows, the counts and the inspector at most once a second, since
a large library takes about a tenth of a second to refresh and the window would otherwise answer the user late. A
fetched page or playlist refreshes them at once. The update's progress shows at once too: a playlist row's phase line,
the status bar's Run part, and the Status cell of a track that starts downloading or moves its ring, which is the only
cell painted again for it. A download that ends keeps its ring, full or where it stopped, until the refresh that shows
the track's new status, so the cell doesn't go blank in between. Everything refreshes as the update ends. The rows of a refresh that an update brings are
made away from the window's thread, so the window keeps answering while a large library's rows are made; the user's
own actions, such as selecting a playlist, show their rows at once.

## Inspector

**One track selected**, top to bottom:

- **Header**: cover art (square, 144 px), **title**, artists, `Album (2021)`. The cover comes from the file;
  tracks without a file show a placeholder.
- **Status**: the status icon and word, plus one line explaining it.
  - Failed tracks add a red reason line, and most add a box with the last two lines of output (Consolas). There are
    three reasons: `Couldn't download` (the box holds yt-dlp's error, or why tagging or placing the file failed), `No match on YouTube Music` (no box), or `Couldn't search YouTube
    Music` (the box holds the error).
  - Needs review tracks replace the explanation with an amber list of what the Matcher doubts, a line for each (see
    the track card below).
  - The **actions** that apply come right under that, so they stay above the fold:

    | State          | Actions                                 |
    |----------------|-----------------------------------------|
    | Needs review   | `Choose match…`, `Edit track…`          |
    | Failed, no match | `Choose match…`, `Retry`              |
    | Failed, other  | `Retry`, `Edit track…`                  |
    | Downloaded, Waiting | `Edit track…`                      |
    | Kept           | `Edit track…`, `Delete file…`           |
    | Removed        | `Keep`, `Delete file…`                  |

    Confirming a match goes through Choose match too: its current match is in the list. The Track menu keeps
    `Confirm match` as a shortcut.
- **Source** (titled **Sources** for a merged track):
  - one line per source: logo, and the name as a link to the source's page;
  - the playlists, as links;
  - the track key;
  - the ISRC.
- **Audio**:
  - for matched tracks: `Matched` (the result's title), `Artists` (the result's, as YouTube Music's search lists them),
    and `Length`, the result's length next to the source length;
  - `ID`, the YouTube Music id as a link; `No match found` in red when the Matcher found nothing;
  - `Chosen by`: `Playlist`, `Matcher` or `Override`;
  - `Confidence`, as a bar + %; the bar is yellow when review is needed;
  - for a YouTube Music entry whose track merged with a Spotify source, `Spotify match: Same ID (96%)` replaces
    the matcher rows.
- **File**: the file name and `Show in folder`.

**Several tracks selected**: `4 tracks selected`, a count per status, and the bulk actions that apply to all of
them, for example `Keep 4 tracks` and `Delete 4 files…`.

So far the track card shows a downloading track as Waiting, without its percentage. It shows the cover the file
holds, the year, and the file's name with `Show in folder`. It shows the
status with its explanation, a failure's reason and output, and the Audio rows of the YouTube Music id that a
YouTube Music source, the Matcher or an override chose. The actions so far are `Choose match…`, for Needs review and
for Failed, no match, `Retry` for Failed, after it, and `Keep` and `Delete file…` for Removed (`Delete file…` alone
for Kept). The ISRC and `Edit track…` come with their features, and so do the selection card's bulk actions; until
then the Track menu's Keep and Delete file act on every selected track they apply to.

```
┌ inspector ──────────────────────┐   ┌ inspector ──────────────────────┐
│          ┌──────────┐           │   │ 2 tracks selected               │
│          │          │           │   │                                 │
│          │    ♫     │           │   │ Status ──────────────────────── │
│          └──────────┘           │   │ Downloaded ✓ 1                  │
│ Best Part                       │   │ Waiting   ◷ 1                   │
│ Daniel Caesar, H.E.R.           │   │                                 │
│ Freudian                        │   │                                 │
│                                 │   │                                 │
│ Status ──────────────────────── │   │                                 │
│ ◷ Waiting                       │   │                                 │
│ Downloads on the next update.   │   │                                 │
│                                 │   │                                 │
│ Sources ─────────────────────── │   │                                 │
│ Listed on ● YouTube Music       │   │                                 │
│           ● Spotify             │   │                                 │
│ Playlists WCS, Slow & bluesy    │   │                                 │
│ Key       ytm:5aM1xdk8lRw       │   │                                 │
│                                 │   │                                 │
│ Audio ───────────────────────── │   │                                 │
│ ID        Not chosen yet        │   │                                 │
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
  (`Freudian (2017)`), or `No album yet` when there's no album.
- **Status**: the section title as in the other cards, 7 px, then the status line: its 16 px icon (downloaded, waiting,
  warn, failed, removed or kept), 6 px, and the word in semibold, on a 16 px line. The explanation follows 3 px below
  in gray, and wraps:

  | Status       | Explanation |
  |--------------|-------------|
  | Downloaded   | The file is in the library and tagged. |
  | Waiting      | Downloads on the next update. |
  | Failed       | Retried on the next update. A failed match says "No match found." instead |
  | Removed      | No longer in any playlist. The file is still in the library. |
  | Kept         | No longer in any playlist, but kept. |
  | Duplicate    | Duplicate of track #12 in this playlist. (The original's position) |

  A duplicate's status line shows the duplicate icon (green once its track is downloaded, as in the table) and
  `Duplicate` whatever its track's status; a failure's reason
  and box still follow, since they are the track's.

  Needs review shows an amber list in the explanation's place: a line for each of the match's doubts, in this order,
  each starting with `• `.

  | Doubt    | Line |
  |----------|------|
  | title    | `Title differs slightly` |
  | artist   | `Other artists` |
  | version  | `Adds "feat. DaBaby"` / `Lacks "Korean"` / `"Arc North" instead of "StayLoose"` |
  | length   | `0:25 longer` / `0:08 shorter` / `Length unknown` |
  | explicit | `Explicit version` / `Clean version` |

  The words in quotes are those of each title that tell the two versions apart, as the title writes them
  (`version_difference`, `arch.md`, Matcher). A match without doubts that still needs review (one from before the
  Matcher stored them) shows the one line `Low confidence`.

  A failed track's reason follows 8 px below in red, on a 16 px line. A failure other than no match adds 4 px below
  the box: a 1 px `#C5C5C5`
  border, 4 px padding at the top and bottom and 6 px at the sides, and the last two lines of the error in Consolas
  at 11/12 of the message font's size, wrapped.

- **Source** (**Sources** when the track has more than one kind of source) holds rows like the other cards':
  - one row per kind of source the track has, YouTube Music first. Its key is `Source`, or with two kinds `Listed on`
    on the first row and nothing on the second. Its value is the logo (16 px), 6 px, and the source's name as a link
    that opens its page in the browser (`https://music.youtube.com/watch?v=<id>`,
    `https://open.spotify.com/track/<id>`);
  - `Playlists`: the track's playlists, in the left panel's order, as links with `, ` between them. A click selects
    that playlist, as a click on its row does. A link that doesn't fit ends in an ellipsis, and the ones after it
    don't show. A track in no playlist reads `None` in gray;
  - `Key`: the row's source key.
- **Audio** holds rows like Source's, by how the row's source got its YouTube Music id, so a duplicate shows what the Matcher
  did with it:
  - a Spotify entry with a match: `Matched`, `Artists`, `Length`, `ID`, `Chosen by` `Matcher`, `Confidence`;
  - a YouTube Music entry: `ID`, `Chosen by` `Playlist`, and `Spotify match` `Same ID (96%)` when the
    track's first Spotify source was matched to that id;
  - a YouTube Music entry whose id is unavailable (`arch.md`, Unavailable ids): first `Listed ID`, the playlist's id
    as a link, then ` (unavailable)` in gray; then the rows of the track's match as for a Spotify entry, with `Length`
    reading ` (playlist 2:54)` in gray instead of ` (Spotify …)`. Before the match, `ID` reads `Not chosen yet`, as
    below;
  - otherwise by where the track's YouTube Music id comes from (`arch.md`, YouTube Music id resolution), and with none, `ID` reads
    `Not chosen yet` in gray, or `No match found` in red after a failed match.

  `Length` is the result's length (`m:ss`), then ` (Spotify 3:32)` in gray. `ID` is the YouTube Music id
  as a link to `https://music.youtube.com/watch?v=<id>`. `Confidence` is a 110 px bar, 6 px tall and centered on the
  line, filled as far as the confidence, then 6 px and the percentage; a narrower card shortens the bar. The fill is
  green, or `#D9A400` (a paused progress bar's yellow) when the track needs review.
- **File**: `Not downloaded yet.` in gray, 7 px under the section title, until there's a file. Then a `Name` row like
  the other cards' (the file's name, ending in an ellipsis when it doesn't fit), and 8 px below it `Show in folder`,
  which opens Explorer on the library folder with the file selected.
- **Cover**: the front cover the file holds, scaled to the square; the placeholder without a file or without a cover
  in it.
- The album line's year is the effective metadata's: Spotify's has none, so it comes from the YouTube Music source's
  info JSON once the track is downloaded.

The selection card: `N tracks selected` in the title's font and line, without a glyph; then **Status**, with a row for
each status that holds any of the tracks, in the order Downloaded, Waiting, Failed, Removed, Kept (a track that needs
review counts under its status): the
word as the key, and its 16 px icon, 4 px, and the count as the value. When all of them are Removed, "No longer in any
playlist. Their files are still in the library." follows 6 px below in gray.

**No playlists yet** (first run): empty.

**Nothing selected**:

- In a playlist scope it shows a playlist card with:
  - glyph and name, URL link, track count;
  - a downloaded/total bar with the counts;
  - how many tracks it shares with other playlists of the same source (shared tracks download once);
  - last update;
  - `Update playlist` and `Remove playlist…`.
- In All tracks it shows the library folder, totals and `Open library folder`.

```
┌ inspector ──────────────────────┐   ┌ inspector ──────────────────────┐
│ ▤ All tracks                    │   │ ● WCS                           │
│                                 │   │   YouTube Music playlist        │
│ Library ─────────────────────── │   │                                 │
│ Folder    C:\Users\…\WestieLib  │   │ Playlist ────────────────────── │
│ Tracks    0 tracks, 0 downloaded│   │ Link      PLGXm7qiSwxfBEFj0x0K… │
│                                 │   │ Tracks    0                     │
│ [Open library folder]           │   │ Downloaded ░░░░░░░░░░░░░░░░ 0/0 │
│                                 │   │ Last update Never               │
│                                 │   │                                 │
│                                 │   │ [⟳ Update playlist] [Remove p…] │
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
  - Library: `Folder` is the library folder in effect, `Tracks` reads like the All tracks row (`0 tracks, 0 downloaded`).
  - Playlist: `Link` is the playlist's ID as a link to its URL (`#0066CC`, underlined while the mouse is over it, the
    hand cursor over its text); a click opens it in the browser. `Tracks` is the count, `Downloaded` a bar like the
    row's that fills the value's width, 6 px, then `downloaded/total`. Then, each only when it isn't zero, a count: `Waiting`,
    `Failed` (in red), `To review` (in amber) and `Shared` (tracks also in other playlists of the same source).
    `Last update` reads like the row without `Updated`: `today 14:03`, `yesterday`, `12 Oct`, or `Never`.
- **Buttons**, 14 px below, 6 px apart, wrapping onto a new row 6 px down when they don't fit: `Open library folder`;
  or `⟳ Update playlist` and `Remove playlist…`. They follow the command rules (see Menu bar and toolbar).

The inspector scrolls vertically when its content is taller than the panel, inside the same padding: 12 px on the
sides and top, 10 px at the bottom. Another card, or the same card for another track or playlist, starts at the top;
a card that only changes (an update reports progress) keeps its place.

## Log pane

- Hidden on the first start; after that, it comes back as it was when the window closed. View › Log or Ctrl+L
  toggles it. It spans the full window width, between the panels and the status bar, and has its own splitter.
- It lists the lines of this run's log file (see `arch.md`, Log), the last 20,000 at most.

```
│ Log   Level [Info and up ▾]   All │ Selected playlist │ Selected track             [Open log folder]  ✕ │
├─────────────────────────┬─────────┬──────────────────┬────────────────────────────────────────────────┤
│ Time                    │ Level   │ Playlist / Track │ Message                                        │
│ 2026-11-12 15:12:01.204 │ Info    │ —                │ Update started: 4 playlists                    │
│ 2026-11-12 15:12:03.871 │ Error   │ Slow & bluesy    │ Couldn't fetch the playlist: HTTP 404 from …   │
│ 2026-11-12 15:13:30.016 │ Warning │ Feeling Good     │ Needs review: "Feeling Good (Live)" by Nina …  │
```

**Toolbar.** A white strip, 28 px tall including a 1 px `#E5E5E5` rule along its bottom. From the left: 8 px, `Log` in
semibold, 12 px, `Level`, 6 px, a drop-down list, 12 px, the scope's flat buttons, then against the right edge
`Open log folder` (a compact button, like the infobar's `Details`), 6 px, and ✕ in a 24 px box, 4 px from the right
edge.

- **Level** picks the least severe level shown; every more severe level shows too. The list holds `Trace and up`,
  `Debug and up`, `Info and up`, `Warnings and up` and `Errors only`, and starts at `Info and up`.

  | Level   | Lines                                                                                       |
  |---------|---------------------------------------------------------------------------------------------|
  | Trace   | each run of a tool: its command line as it starts; as it ends, its exit code when that isn't 0, then each line it wrote to stderr (and to stdout when it failed) |
  | Debug   | details for finding a fault: each whole percentage of a download's progress that yt-dlp reports, each search as it starts and ends, with its result count, and its best result |
  | Info    | what the app did: an update started and finished, a playlist synced, a track matched, a tool installed, a playlist added. Lines state what happened, never what to do |
  | Warning | something the user may want to look at: a match that needs review, a fetch that waits for a tool, a search YouTube Music refused |
  | Error   | something that failed: a fetch, a match, a tool install, saving the state                    |

- **Scope** is three flat buttons, `All │ Selected playlist │ Selected track`, drawn like the status filter's (see
  Main panel), 20 px tall. One is always selected; it starts at `All`.
  - `Selected playlist` shows the lines about the selected playlist and about the tracks it lists now. While All
    tracks is selected, it shows every line, as All tracks is the whole library.
  - `Selected track` shows the lines about the selected tracks: with several rows selected, the lines of any of them.
    Without a selected track the list is empty and reads `Select a track to see its lines.` in gray.
  - The scope follows the selection: selecting another playlist or track changes the lines.
  - When two tracks merge, the lines of the merged one go with the track that survives.
- `Open log folder` is Tools › Open log folder. ✕ hides the pane, like View › Log.
- The level and the scope are saved with the window state.

**List.** A list view with the columns Time (150 px), Level (70 px), Playlist / Track (160 px) and Message, which takes
the rest of the width. Rows are 20 px tall.

- Time is the local date and time to the millisecond: `2026-11-12 15:12:03.871`.
- Level reads `Trace`, `Debug`, `Info`, `Warning` or `Error`. Warning rows are amber (`#9D5D00`) and Error rows red
  (`#C42B1C`), in every column; the other rows are black.
- A message fills its column however long it is, and one too long for it ends in an ellipsis at the column's right
  edge. Hovering such a message shows all of it in a tooltip, wrapped at the column's width. (The list view itself
  cuts a cell's text at 259 characters, wherever that falls, so while it draws a row, the list gets an empty Message
  cell and draws it like the others, background, hover and selection included; the app then draws the message over
  it. That holds for every row the list draws, also one it redraws alone as the mouse moves onto it, so a message is
  never drawn twice. The list also goes through a row's drawing to measure it, without drawing; the app draws nothing
  then.)
- Playlist / Track is the track's title when the line is about a track, else the playlist's name when it's about a
  playlist, else `—`. It is the name when the line was written, so it stays after the playlist is removed.
- New lines scroll the list to the bottom when it showed the last line; once the user has scrolled up, it stays.
  Changing the level or the scope scrolls to the bottom.
- The selection stays on its lines when the lines shown change, as with another level, scope, playlist or track, or as
  the oldest lines go. A selected line that is no longer shown leaves the selection.
- The hover highlight stays on the row under the mouse when the rows move while the mouse stands still: as new lines
  scroll the list, or as a key or the wheel scrolls it. (The list view itself only looks for the row under the mouse
  when the mouse moves, which left the highlight on a row that had moved away.) This holds for every list view in the
  app.

**Going to a line's track or playlist.** A double-click on a line goes to what it is about:

- A line about a track selects that track in the table, alone, and scrolls it into view. When the selected playlist
  doesn't list it, or the search, the source or the status tab hides it, All tracks is selected first, with the
  search emptied, the source at All and the All tab, so the track shows. In a playlist that lists the track twice, the
  first row is selected.
- A line about a playlist selects that playlist in the left panel, as a click on it would.
- A line about neither does nothing, and so does a line whose track or playlist is no longer in the library; that
  beeps.
- The keyboard focus stays in the log, and the log's own selection stays on the line. In the `Selected track` and
  `Selected playlist` scopes the lines then follow the new selection, as they always do.

**Context menu.** A line's context menu has **G**o to track, Go to **p**laylist, ─, **C**opy (Ctrl+C).

- Go to track is enabled for a line about a track, Go to playlist for one about a playlist; each does what a
  double-click does. They act on the line that was right-clicked, or with the keyboard on the focused line.
- **Copy** puts the selected lines on the clipboard, one per line, with their four columns as the list shows them,
  separated by tabs. Ctrl+C in the list does the same. When another program holds the clipboard, it beeps instead.
- A right-click selects the line as in the track table (a line that isn't selected alone, a selected one keeps the
  selection), and gives the list the focus. A right-click below the lines shows no menu.
- Shift+F10 and the menu key open it while the list has the focus, at the bottom-left corner of the focused line, or
  of the first selected line, which scrolls into view first. With no line selected they show nothing.

## Status bar

| Part    | Content |
|---------|---------|
| Run     | `Updated today 14:05` (`Ready` before any update), or the phase of the running update with an embedded 120 px progress bar against the part's right edge: `Syncing 1 of 2` while it syncs playlists (the first one being synced), `Matching 6 of 25` while it matches tracks, `Downloading 6 of 25` while it downloads them (the ones downloading right now count, as for a playlist); or `⧗ Waiting for ffmpeg` while all that's left of the run waits for tools |
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
  | Run      | 22%     | 250 px  | `Downloading 12 of 400` and its 120 px bar                   |
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
    | `Waiting for yt-dlp`, in gray                   | ⧗         | a YouTube Music link, while yt-dlp isn't installed | off |
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
  - An `Update now` check box (on by default). With it, `Add` also updates the new playlist; during an update it
    joins the update's queue.
  - `Add` (default, enabled only once the look-up found a playlist that isn't in the list yet) and `Cancel`. `Cancel`
    also cancels the look-up.
  - `Add` stores the playlist with the name the look-up found. The welcome screen gives way to the main panel, and the
    playlist shows in the left panel.
- **Choose a match**. Opened by `Choose match…` in the inspector, the Track menu or Edit track. One dialog
  covers both the case with several candidates and the case where nothing was found:
  - At the top: the source track (glyph, title, then `by <artists> (<album>, <length>)` in gray), with a play button
    at the right end of the line (see Play buttons), which opens the track on Spotify. Under it the track's current
    match, by where it comes from: "Current match: <title>, 58% confidence", "Current match: <title>, from a YouTube Music
    playlist", "Current match: <title or id>, chosen by you", or without one "No match found." (after a failed match)
    or "Not matched yet."
  - `Search YouTube Music:` an edit pre-filled with the Matcher's first query, and `Search`. The search is the
    Matcher's own (`arch.md`, Matcher › Search): songs only, and no tool needed. Anything that isn't a song is
    chosen by its link (below).
  - A results list with Title | Artists | Album | Length | Match and an untitled last column with each row's play
    button, which opens that result on YouTube Music without selecting or choosing the row. Title takes the width the
    other columns leave. The rows come in the Matcher's ranking (`arch.md`, Matcher › Verdict; ties in the order
    below), each YouTube Music id once:
    - the current match, marked `(current)`;
    - the YouTube Music sources the playlists list that the Matcher doesn't reject for the track, marked
      `(in library)`;
    - the search results. When the dialog opens, those of the Matcher's queries; after `Search` (or Enter in the
      edit), those of the edit's text. The current match and the library rows stay.
    - A result whose YouTube Music id already belongs to another track is marked `(in library)` too. Choosing a
      result in the library merges the track into the one that has it.
    - `Match` is the Matcher's verdict on it (`judge_match`): `Same recording` when it would accept it; `Check`
      and its doubts when it would ask for a review (`Check length`, `Check artist, version`), in the order of the
      track card's sentences; `Other recording`, `Other artist` or `Other song` when it would reject it. Lengths more
      than 5 s off the source length are amber.
    - While a search runs, the list is empty and reads "Searching YouTube Music…". Without rows after it, it reads
      "No results. Try fewer words, or paste a link below.", or "Couldn't search YouTube Music: <reason>" when the
      search failed.
  - `Or paste a YouTube or YouTube Music link:` an edit with a play button on its right, enabled once the link names
    a YouTube Music id, and a live detection line under it. The line reads nothing for an empty edit, `Not a YouTube link` in
    gray, or the YouTube Music logo and `YouTube Music id <id>`, followed by the title when the library has that id.
    The links it takes are in `arch.md` (YouTube Music links).
  - A link that names a YouTube Music id wins over the list's selection; selecting a row leaves the link as it is.
  - The footer has `Use this match` (default) and `Cancel`. `Use this match` is enabled once the link names a YouTube
    Music id or a row is selected. A double-click on a row is `Use this match`.
  - `Use this match` on the current match confirms it. Any other result becomes the override, which downloads
    the track again.
  - While it is open, an update starts no new downloads, because YouTube Music doesn't answer the dialog's searches
    while yt-dlp downloads. The downloads under way go on, and the rest wait until the dialog closes.
  - **Play buttons** open a track's page in the browser, so it can be heard before it's chosen. Each is a small
    button with the 12 px play glyph and a tooltip: `Play on Spotify` for the track (`Play on YouTube Music` for a
    YouTube Music entry whose id is unavailable, which opens its page though it won't play), `Play on YouTube Music`
    for a result or a link. Such an entry's heading shows the YouTube Music logo.
    - The track's and the link's are push buttons, as tall as the search field. The link's grays out with its glyph
      while the link names no YouTube Music id.
    - A row's is painted in its cell: just the glyph, and on hover a box with the hover colors of the playlist rows'
      update control (`#E5F3FF`, `#CCE8FF` frame, `#99D1FF` on a selected row), `#CCE4F7` while pressed. It reacts like
      a button: it opens the result when the mouse is released over it, and a double click on it is two clicks, never
      `Use this match`.
  - **Size.** It opens at 900 × 600 px and is resizable, with a size grip in the bottom right corner, down to the
    size of its template, about 660 × 490 px. The list takes the extra height and its Title column the extra width; the fields and lines take the
    extra width, and the buttons keep to their edges. The size is kept for the next time, and saved with the window
    (see Window layout).
  - At its smallest:

    ```
    ┌ Choose a match ─────────────────────────────────────────────────────────┐
    │ ● Best Part by Daniel Caesar, H.E.R. (Freudian, 3:29)               [▶]  │
    │ Current match: Best Part, 75% confidence                                 │
    │ Search YouTube Music: [Daniel Caesar H.E.R. Best Part        ] [Search]  │
    │ ┌ Title          │ Artists       │ Album    │ Length │ Match          │  ┐│
    │ │ Best Part (in …│ Daniel Caesar │          │ 3:29   │ Same recording │▶ ││
    │ │ Best Part (cur…│ Daniel Caesar │ Freudian │ 3:56   │ Check length   │▶ ││
    │ │ Best Part - Li…│ Daniel Caesar │ Live     │ 4:12   │ Other recording│▶ ││
    │ └──────────────────────────────────────────────────────────────────────┘│
    │ Or paste a YouTube or YouTube Music link:                                │
    │ [                                                               ] [▶]    │
    │ ● YouTube Music id 5aM1xdk8lRw                                           │
    │                                                                          │
    │                                            [Use this match]  [Cancel]  ◢ │
    └──────────────────────────────────────────────────────────────────────────┘
    ```
  - So far it opens from the inspector, the Track menu, the row's context menu and a double-click on a row that needs a
    match; Edit track comes with its feature.
- **Edit track**. Opened by `Edit track…`, Enter, or a double-click on a row that needs no match:
  - **Cover art**: a 132 px preview, `Choose image…`, `Paste` and `Reset`, plus a source line (`From the
    YouTube thumbnail`, `From Spotify`, `Custom image`).
  - **Tags**: Title, Artists, Album and Year edits. A changed field gets a `Reset` link, and the original value
    shows under it in gray.
  - **Audio**: `YouTube link:` edit and `Choose match…`, with the note "A different link downloads the audio
    again."
  - `Save` (default) and `Cancel`. Saving re-tags the file and renames it when the artists or title changed.
    Edited values survive later updates.
- **Settings**, as group boxes:
  - **Library**: a read-only folder edit and `Browse…`.
  - **Files**: `Format:` radio buttons Opus / FLAC, and an `Embed album art in files` check box (on by default).
    Both apply to new downloads; existing files stay as they are.
  - **Downloads**: `Parallel downloads` spin (1–4, 2 by default) and `Pause between tracks` spin (seconds, 1 by default).
  - **Tools**: a list of tool, release and status. Under it, an `Update tools automatically` check box (on by
    default), `Check for updates`, and "Last checked …".
    - One row per download: `yt-dlp`, `ffmpeg` (ffmpeg brings ffprobe along) and `deno`. The release is empty for a tool
      that isn't installed.
    - The status is an icon and a word, as in the Tools part: ✓ `Up to date` (checked since the app started) or
      `Installed`; ⟳ `Checking…`, `Downloading 45%` or `Installing…`; ⚠ `Not installed` or `Failed: <error>`.
    - The list follows the tools while the dialog is open.
    - `Check for updates` starts at once, without waiting for `OK`, and updates installed tools even when the check
      box is off. It is disabled as in the Tools menu.
    - "Last checked" reads like a playlist's last update: `Last checked today 14:03`, `Last checked yesterday`,
      `Last checked 12 Oct` (`12 Oct 2025` in another year), or `Never checked`.
  - **Maintenance**: `Rebuild library from files…`.
  - The footer shows the version on the left, and `OK` / `Cancel`.
  - So far the dialog has the Library, Files, Downloads and Tools groups; Maintenance comes with its feature. A folder chosen
    with `Browse…` and the check box take effect on `OK`.
- **About** is a TaskDialog with the app icon, `WestieLib` as the main instruction, `Version 1.2.0` (the build's
  version) and OK.
- **Confirmations** use a TaskDialog:
  - Remove playlist: title `Remove playlist`, main instruction `Remove "<name>"?`, content "Its downloaded files stay in
    the library." Buttons `Remove` and `Cancel`, with Cancel the default. Tracks left in no playlist
    become Removed, and the infobar offers to review them afterwards.
  - Delete file: title `Delete file`, main instruction `Delete "<file name>"?`, or `Delete 4 files?` for several,
    content "The file is deleted from the library folder, not moved to the Recycle Bin." ("The files are" for
    several). Buttons `Delete` and `Cancel`, with Cancel the default. It counts the selected tracks that are Removed
    or Kept, which are the ones it deletes; Keep, without a confirmation, makes the selected Removed ones Kept.
  - Rebuild says it replaces track entries.
- **First run** has no playlists, so the main panel shows its welcome screen (see Main panel › First run). Tools install
  in the meantime, and the status bar shows their progress.

## Command and event mapping

| Core API | UI |
|----------|----|
| `addPlaylist` | toolbar, left panel button, File menu, first-run button → Add playlist dialog |
| `lookUpPlaylist` / `PlaylistLookedUp` | Add playlist dialog's detection line |
| `removePlaylist` | playlist context menu, File menu, inspector playlist card → confirmation |
| `sync` / `cancel` | toolbar split button (Update all), Update menu, playlist row update button, playlist context menu, inspector card |
| `SyncQueued` / `PlaylistFetching` / `TracksQueued` / `TrackMatching` / `SyncFinished` | playlist row phase, status bar Run part |
| `resolveRemovals` | infobar → Removed tab, Track menu and row context menu, inspector (single and bulk) |
| `retry` | inspector Retry, Track menu and row context menu |
| `confirmMatch` | inspector actions, Track menu and row context menu |
| `setYtmOverride` | Choose a match dialog (from the inspector, the Track menu, the row context menu or a double-click on a row that needs a match), Edit track dialog (Audio link) |
| `editTrack` | Edit track dialog (tags, cover art) |
| `searchCandidates` / `CandidateSearchResults` | Choose a match dialog's list |
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
| Toolbar | `TOOLBARCLASSNAME` with `TBSTYLE_FLAT \| TBSTYLE_LIST` and an image list; `BTNS_DROPDOWN` + `TBSTYLE_EX_DRAWDDARROWS` for Update all |
| Infobar | custom-painted child window holding push buttons and a close button |
| Close button (infobar, log) | a small custom-painted window with the ✕ icon; hover `#E5F3FF`, pressed `#CCE4F7`, hot-tracked with `TrackMouseEvent`; it takes the mouse capture on `WM_LBUTTONDOWN` and acts on `WM_LBUTTONUP` inside it |
| Log toolbar | a custom-painted strip (the rule) holding labels, a `CBS_DROPDOWNLIST` combo box, the status filter's `WC_TABCONTROL` with flat buttons, a compact push button and a close button |
| Status bar | `STATUSCLASSNAME` with owner-drawn parts, an embedded `PROGRESS_CLASS` and a `TOOLTIPS_CLASS` tool over the Tools part |
| Left panel | All tracks is a custom-painted row window above the list. The playlist list is a custom-painted window with its own vertical scroll bar; rows, bars and icons are drawn in its `WM_PAINT`. It isn't a `LISTBOX`, because list box items are at most 255 px tall, which a playlist row passes above ~260% scaling. `No playlists yet.` is a label, and `+ Add playlist` a push button with an icon; both are children of the list, placed after the last row, so they scroll with it. Rows hot-track with `WM_MOUSEMOVE` + `TrackMouseEvent` (`WM_MOUSELEAVE`); `WM_LBUTTONDOWN` presses the row it hits, takes the mouse capture and the focus, and `WM_LBUTTONUP` over the same row selects it. The list's `WM_KEYDOWN` moves the selection. `WM_CONTEXTMENU` selects the row and shows the context menu with `TrackPopupMenu` (`TPM_RETURNCMD`). The per-row update control is drawn in the same pass, right after the measured line-5 text (`GetTextExtentPoint32`), with the hover colors `#E5F3FF` and a `#CCE8FF` frame (`#99D1FF` on a selected row) while the mouse is over it. It is hit-tested in `WM_LBUTTONDOWN`, takes the mouse capture until `WM_LBUTTONUP`, hot-tracked with `WM_MOUSEMOVE` + `TrackMouseEvent`, and has a tooltip-control tool per row rectangle while it can be clicked. The spinning ⟳ is 12 icon resources (30° steps) advanced by a ~100 ms `WM_TIMER` that invalidates only the icon rectangles. The timer runs only while a row spins. When `SPI_GETCLIENTAREAANIMATION` is off, the icon stays still. The inspector's `Downloading` icon uses the same frames |
| First run | a panel that paints the icon, with labels, an `EDIT` (`ES_AUTOHSCROLL`; the hint follows `EN_CHANGE`) and push buttons; the picker is `IFileOpenDialog` with `FOS_PICKFOLDERS` |
| Filter bar | `EDIT` with `EM_SETCUEBANNER`, holding the magnifier and the ✕ (a close button) as child windows, with `EM_SETMARGINS` leaving room for both; a `CBS_DROPDOWNLIST` combo box |
| Status filter | `WC_TABCONTROL` with `TCS_BUTTONS \| TCS_FLATBUTTONS`; labels rewritten with counts. The control's own buttons are classic 3D ones on a gray band, so its subclass paints the buttons, the hover and the separators in `WM_PAINT`, and the control keeps the clicks, keys and selection |
| Track table, log | `WC_LISTVIEW` with `LVS_REPORT \| LVS_OWNERDATA`, `LVS_EX_FULLROWSELECT \| LVS_EX_DOUBLEBUFFER \| LVS_EX_HEADERDRAGDROP`; `NM_CUSTOMDRAW` for the Status icon and colored text; `SetWindowTheme(L"Explorer")`, which also draws faint lines under the column dividers. The header's `NM_CUSTOMDRAW` post-paint draws the rule under the titles, which Explorer's design leaves out. The progress ring comes from a 24-step image list. The Status tooltip uses `LVS_EX_INFOTIP` + `LVN_GETINFOTIP`. The # column is inserted when a playlist is selected and removed for All tracks. The log colors its rows through `clrText` at `CDDS_ITEMPREPAINT`, keeps to the bottom with `LVM_ENSUREVISIBLE`, and takes its empty text from `LVN_GETEMPTYMARKUP`. Both act on `NM_DBLCLK`, and on the `WM_CONTEXTMENU` the list passes to its parent, with `TrackPopupMenu` (`TPM_RETURNCMD`); the log's Ctrl+C comes as `LVN_KEYDOWN`. After rows move under a mouse that stands still, `LVM_SUBITEMHITTEST` at the cursor and `LVM_SETHOTITEM` move the hover |
| Inspector | a child window with its own vertical scroll bar, holding one card at a time. A card is a panel of labels, buttons and small self-painted windows: the title, the section titles with their rules, the downloaded bar and the link. The link isn't `WC_LINK`, which can't end in an ellipsis and takes its background from its parent; it hot-tracks like the playlist rows. Covers decoded with WIC into an HBITMAP |
| Dialogs | `DIALOGEX` resources, `UPDOWN_CLASS`, an `SS_OWNERDRAW` static for each detection line (icon and text), a `WM_TIMER` that delays the playlist look-up and another that redraws its spinner about 60 times a second (drawn with GDI+, its angle from the time since it started), `BS_AUTORADIOBUTTON`, `IFileOpenDialog` (`FOS_PICKFOLDERS` for the library, image filter for covers), `TaskDialogIndirect`. The Choose a match results are a report ListView, and its empty text comes from `LVN_GETEMPTYMARKUP`; the rows' play buttons are painted at `CDDS_ITEMPOSTPAINT`, and the list is subclassed for their mouse handling and tooltip. Choose a match has `WS_THICKFRAME`, an `SBS_SIZEGRIP` scroll bar, `BS_ICON` push buttons, and moves its controls on `WM_SIZE` by their anchors; `WM_GETMINMAXINFO` holds its minimum. Edit track's cover is an `SS_BITMAP` static, and Paste reads `CF_DIB` |
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
