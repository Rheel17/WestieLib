# WestieLib architecture

WestieLib keeps online playlists in sync with a local folder of tagged audio files (Opus by default, FLAC as an
option) for use in Mixxx. It reads YouTube Music and Spotify playlists, finds and downloads the audio from YouTube
Music, tags every file with title, artists, album and cover art, and remembers what it has done so each sync only
processes what changed.

It is a single, statically linked Win32 executable written in C++26.

## Components

```
                         +------------------+
                         |     UI layer     |  Win32 windows
                         +------------------+
                        commands |      ^ events (PostMessageW)
                                 v      |
+--------------------------------------------------------------------------+
|                                  Core                                    |
|                                                                          |
|   SyncEngine ----> PlaylistSource (YouTubeMusicSource, SpotifySource)    |
|       |      ----> Matcher                                               |
|       |      ----> Downloader ----> Tagger ----> Library                 |
|       |                                                                  |
|   StateStore      Settings      ToolManager      SelfUpdater   Log       |
|                                                  RemoteConfig            |
+--------------------------------------------------------------------------+
                                 |
                                 v
+--------------------------------------------------------------------------+
|                       Platform services (Win32)                          |
|   HttpClient   ProcessRunner   FileOps   Hasher   TextNormalizer         |
+--------------------------------------------------------------------------+
```

- **Core** holds all logic. It reaches the operating system only through the platform service interfaces,
  which makes every core component unit-testable with fakes.
- **Platform services** implement those interfaces with Win32 APIs.
- **UI layer** calls core commands and receives core events. All window code lives here.

## Files and folders

```
<anywhere>\WestieLib.exe                 the application; runs from any folder

%LOCALAPPDATA%\WestieLib\
                                          (or the folder WESTIELIB_DATA_DIR names; the UI checks use that)
    settings.json                         library location and sync tuning
    state.json                            playlists and tracks (human-readable)
    state.json.bak                        previous version of state.json
    remote.json                           cached copy of the remote config
    covers\<track key>.jpg                custom cover art set through editTrack (':' in the key becomes '_')
    tools\
        tools.json                        installed tool versions and digests
        yt-dlp.exe
        ffmpeg.exe
        ffprobe.exe
        deno.exe
    work\<track key>\                     per-track scratch space during download
    logs\<timestamp>.log                  one log per run, including the tool runs; the 20 newest are kept

<library folder>\                          flat folder of finished .opus / .flac files

HKCU\Software\WestieLib\Window            main window placement and panel widths (UI layer)
```

## Data model

### settings.json

```json
{
  "version": 1,
  "libraryPath": "D:\\Music\\WestieLib",
  "format": "opus",
  "embedCover": true,
  "concurrency": 2,
  "delayBetweenTracksSeconds": 1,
  "autoUpdateTools": true
}
```

| Field                       | Meaning                                                                       |
|-----------------------------|-------------------------------------------------------------------------------|
| `version`                   | the file's layout, 1 for now                                                  |
| `libraryPath`               | the library folder; `Music\WestieLib` in the user's profile by default        |
| `format`                    | `opus` or `flac`; applies to new downloads, existing files keep their format  |
| `embedCover`                | write the cover art into the file                                             |
| `concurrency`               | number of parallel downloads (1–4)                                            |
| `delayBetweenTracksSeconds` | pause between track starts; 1 by default                                      |
| `autoUpdateTools`           | update installed tools at startup (see ToolManager)                           |

The file is pretty-printed UTF-8, indented by 4 spaces. Keys it doesn't know are skipped and missing ones keep their
defaults, so files from other versions still load; a `format` it doesn't know reads as `opus`, and `concurrency` is
held to 1–4. Without a file the app runs on the defaults and writes nothing until a setting changes.

### state.json

Pretty-printed UTF-8, read and written with glaze. Fields appear in struct declaration order. `playlists`,
`sources` and `tracks` are `std::map`s, so their keys are sorted. File paths are relative to `libraryPath`.

The state separates what playlists list from what is on disk:

- A **source** is one entry in an online catalog: a YouTube Music entry or a Spotify track. Its key is
  `<source>:<source id>`, e.g. `ytm:<ytmId>` or `spotify:<trackId>`. Playlists list source keys, and a source
  that appears in several playlists has one entry.
- A **track** is one audio file in the library, made from one YouTube Music id. It has one or more sources. The
  same song in a YouTube Music playlist and a Spotify playlist is one track with two sources and one file.

```json
{
  "version": 1,
  "playlists": {
    "ytm:PLxxxxxxxx": {
      "url": "https://music.youtube.com/playlist?list=PLxxxxxxxx",
      "name": "WCS",
      "addedAt": "2026-10-11T14:02:00Z",
      "lastSync": "2026-10-11T14:03:00Z",
      "sources": [ "ytm:aaaaaaaaaaa", "ytm:ccccccccccc" ]
    },
    "spotify:37i9dQZF1DXxxxxxxx": {
      "url": "https://open.spotify.com/playlist/37i9dQZF1DXxxxxxxx",
      "name": "Her playlist",
      "addedAt": "2026-10-11T14:04:00Z",
      "lastSync": "2026-10-11T14:05:00Z",
      "sources": [ "spotify:6r8k1vznHrzlEKYxxxxxxx", "spotify:7yq3vB0cNvb1Xxxxxxxxxx" ]
    },
    "spotify:5aX8kq1sTn2Yz9wxxxxxxx": {
      "url": "https://open.spotify.com/playlist/5aX8kq1sTn2Yz9wxxxxxxx",
      "name": "Slow & bluesy",
      "addedAt": "2026-10-09T20:10:00Z",
      "lastSync": "2026-10-09T20:12:00Z",
      "fetchError": "HTTP 404 from api-partner.spotify.com",
      "fetchErrorAt": "2026-10-11T14:05:00Z",
      "sources": [ "spotify:6r8k1vznHrzlEKYxxxxxxx" ]
    }
  },
  "sources": {
    "ytm:aaaaaaaaaaa": {
      "title": "Title",
      "artists": [ "Artist" ],
      "album": "Album",
      "year": 2021,
      "durationMs": 215000,
      "track": "ytm:aaaaaaaaaaa"
    },
    "ytm:ccccccccccc": {
      "title": "Title",
      "artists": [ "Artist One" ],
      "durationMs": 243000,
      "track": "spotify:6r8k1vznHrzlEKYxxxxxxx"
    },
    "spotify:6r8k1vznHrzlEKYxxxxxxx": {
      "title": "Title",
      "artists": [ "Artist One", "Artist Two" ],
      "album": "Album",
      "year": 2017,
      "durationMs": 242733,
      "explicit": false,
      "isrc": "USxxx0000000",
      "coverUrl": "https://i.scdn.co/image/...",
      "match": { "ytmId": "ccccccccccc", "confidence": 1,
                 "title": "Title", "artists": [ "Artist One" ], "album": "Album", "durationMs": 243000,
                 "explicit": false },
      "track": "spotify:6r8k1vznHrzlEKYxxxxxxx"
    },
    "spotify:7yq3vB0cNvb1Xxxxxxxxxx": {
      "title": "Title (WCS Edit)",
      "artists": [ "Artist" ],
      "album": "Title",
      "year": 2024,
      "durationMs": 228000,
      "track": "spotify:7yq3vB0cNvb1Xxxxxxxxxx"
    }
  },
  "tracks": {
    "ytm:aaaaaaaaaaa": {
      "sources": [ "ytm:aaaaaaaaaaa" ],
      "ytmId": "aaaaaaaaaaa",
      "edits": { "album": "Album (Deluxe)" },
      "status": "downloaded",
      "file": "Artist - Title.opus",
      "addedAt": "2026-10-11T14:03:00Z"
    },
    "spotify:6r8k1vznHrzlEKYxxxxxxx": {
      "sources": [ "spotify:6r8k1vznHrzlEKYxxxxxxx", "ytm:ccccccccccc" ],
      "ytmId": "ccccccccccc",
      "status": "downloaded",
      "file": "Artist One, Artist Two - Title.opus",
      "addedAt": "2026-10-11T14:05:00Z"
    },
    "spotify:7yq3vB0cNvb1Xxxxxxxxxx": {
      "sources": [ "spotify:7yq3vB0cNvb1Xxxxxxxxxx" ],
      "status": "failed",
      "failure": "noMatch",
      "error": "Searched ISRC QZxxx2400000: no results\nSearched \"Artist Title\": no usable results",
      "addedAt": "2026-10-11T14:05:00Z"
    }
  },
  "unavailableYtmIds": [ "eeeeeeeeeee" ]
}
```

In this example, the WCS playlist (YouTube Music) and Her playlist (Spotify) both contain "Title". The Matcher
picked the same YouTube Music id (`ccccccccccc`) that WCS lists, so the two sources share one track and one file.

**Playlist fields.**
- The key is `ytm:<list id>` or `spotify:<playlist id>`, and `url` is the canonical link built from it (see Playlist
  links), whatever form the user pasted.
- `name` comes from the look-up when the playlist is added (see Playlist look-up), and each fetch replaces it.
  Playlists stored by an earlier version may have an empty name until their first fetch.
- `addedAt` is when the playlist was added. The UI lists playlists in that order.
- `sources` is the ordered list of source keys.
- `fetchError` and `fetchErrorAt` are set when the last fetch failed, and cleared by the next successful fetch.
- `lastSync` keeps the time of the last successful sync.

**Source fields.**

| Field                            | Meaning                                                                    |
|----------------------------------|----------------------------------------------------------------------------|
| `title`, `artists`, `durationMs` | From the playlist                                                          |
| `album`, `year`                  | Spotify: the album from the playlist response; its answer has no year. YouTube Music: from the info JSON (`album`, `release_year`), once the source has been downloaded |
| `explicit`                       | Spotify only: whether the track is explicit (`contentRating.label` `EXPLICIT`); left out when unknown |
| `isrc`, `coverUrl`               | Spotify only. The playlist response has the cover but no ISRC; where the ISRC comes from is decided with the Matcher |
| `match`                          | Spotify, and a YouTube Music source whose own id is unavailable (see Unavailable ids): the Matcher's result, with `ytmId`, `confidence` (0 to 1), `confirmed` (left out unless the user confirmed the match), `doubts` (why it needs review: `title`, `artist`, `version`, `length`, `explicit`; left out when there are none), and the result's `title`, `artists`, `album`, `durationMs` and `explicit` as YouTube Music lists them, so a review shows what was matched |
| `track`                          | Key of the track this source belongs to                                    |

**Track fields.**

| Field              | Meaning                                                                         |
|--------------------|---------------------------------------------------------------------------------|
| `sources`          | Its source keys, in the order they joined. The first one created the track; its key is the **track key**, which never changes |
| `ytmId`          | The YouTube Music id the file was made from                                     |
| `ytmOverride`    | See YouTube Music id resolution                                                 |
| `edits`            | User edits from `editTrack`: any of `title`, `artists`, `album`, `year`, and `cover: true` for a custom cover in `covers\`. They win over source values, and syncs never overwrite them |
| `status`           | See below                                                                       |
| `failure`, `error` | Set with `failed`. One of `noMatch` (no usable candidate; `error` lists the searched queries), `search` (the search itself failed, for example without a network; `error` holds why), or `download` (a tool failed; `error` holds its last output lines) |
| `file`             | File name in the library                                                        |
| `addedAt`          | When the track entered the state                                                |

**Track status.**

| Status       | Meaning                                                                |
|--------------|------------------------------------------------------------------------|
| `pending`    | Known from a playlist, waiting to be downloaded                        |
| `downloaded` | File is in the library, tagged                                         |
| `failed`     | Last attempt failed; `failure` and `error` hold the reason; retried on the next sync or by `retry` |
| `orphaned`   | None of its sources is in a playlist anymore; waiting for a keep/remove decision |
| `kept`       | None of its sources is in a playlist anymore; kept by choice           |

**YouTube Music id resolution.** A track's YouTube Music id is the first of:
1. its `ytmOverride`;
2. the id of its first YouTube Music source whose id isn't unavailable, because the playlist owner chose it;
3. the Matcher's result for its first Spotify source;
4. the Matcher's result for its first YouTube Music source (see Unavailable ids).

A match whose id is unavailable doesn't count: the track is matched again.

**Unavailable ids.** YouTube Music keeps listing ids that it no longer plays here: a release that a label delivered
again under new ids, whose old uploads went unlisted, or one blocked in this country. A playlist keeps such an id,
and the search can still return one. yt-dlp then fails with `Video unavailable. This video is not available`
whatever client it uses, and signing in doesn't change that. `unavailableYtmIds` (a sorted set) lists the ids a
download found unavailable:
- A download fails as unavailable (`is_unavailable_error`, `src/core/download/UnavailableError`) when an `ERROR:`
  line says `Video unavailable`, `This video is not available`, `not made this video available in your country`,
  `This video has been removed`, or `only available to Music Premium members`. The app doesn't sign in, so Premium is
  unavailable too. An age check (`Sign in to confirm your age`), a bot check, or a network error isn't: those are
  ordinary download failures.
- Unless the id is the track's override (the user's choice, which fails as an ordinary download), it joins
  `unavailableYtmIds`; a match that gave it is removed from its source; and the track goes back to
  `pending` without a failure, so the run matches it (SyncEngine › So far). Its sources' facts don't change, so the
  match searches with its first Spotify source's, or, with none, its first YouTube Music source's title, artists
  and length. That source has no album and no explicit flag, so only a Same name, Same artists, the same version and a
  Same or Close length make an Accept; anything less is a review, never a guess.
- The Matcher leaves unavailable ids out of every candidate list: the search's, the library's (The library first) and
  the Choose a match dialog's. A YouTube Music source whose id is unavailable isn't a candidate for the library either.
- The set only grows. An id that becomes available again stays out until the user deletes it from the state; that
  costs a review or a second upload, never a failed download on every sync.
- The track keeps its key and sources, so its playlists still list it. Its file is made from the match's id; when
  another track already has that id, the two merge as for any match.

`resolved_ytm_id` (`src/core/state/TrackYtmIds`) gives the id, the source it comes from (for an override, the
track's first source), and whether it is the override.

A track **needs review** when its YouTube Music id comes from a match that isn't `confirmed` and whose confidence is under the
review threshold (see Matcher › Choose). The flag isn't stored, so a new threshold applies to the matches already
made. Resolution runs for tracks without a file, and again when the override changes. Setting `ytmOverride`
re-downloads the track (or merges it, see below), and clears `failure`; the track's YouTube Music id then no longer comes from
the match, so it no longer needs review.

A track that needs review doesn't download: its match may be another recording, and a wrong file in the library is
worse than a track that waits. It stays `pending` until `confirmMatch` confirms the match or an override is set.

`needs_review` (`src/core/state/MatchReview`) gives it, for a match and for a track.

**Merging.** No two tracks share a YouTube Music id. An id gets resolved in three ways:
- a fetch adds a YouTube Music source;
- the Matcher finishes;
- an override is set.

Each time, if another track already has that id, the two tracks merge. A track that needs review doesn't count:
its match may be wrong, so it neither merges into a track with its id nor takes one in, until `confirmMatch`
confirms it, which then merges it. Meanwhile two tracks can share an id. A merge goes like this:
- The track that had the id first survives. The other track's sources move into it, and its entry is
  deleted.
- The survivor keeps one file. It takes the deleted track's file, with its `ytmId`, status and failure, when it has
  none of its own, or when only the deleted track's file was made from the id they share. The file it doesn't keep is
  deleted (Library › Files follow the state); nothing goes to the Recycle Bin.
- The surviving track keeps its own `edits`, and takes the other track's edits for fields it hasn't edited.
- When the merge changes the tags the file should have, the file is re-tagged, and renamed when its name changes.
- The engine emits `TracksMerged`.

Sources stay attached to their track until the track is removed, with one exception: choosing another result for one
entry of a merged track (Choose a match, `setYtmOverride` on a duplicate, with manual matching) moves that source out
into a track of its own, which gets the override. So a merge is never final.

`set_ytm_override` (`src/core/matching/YtmOverride`) does it for an entry, a source, and a YouTube Music id:
- The id the track has already changes nothing, except that a match that needs review is confirmed
  (`confirm_match`).
- When the source is the one the track's id comes from, the override is the whole track's. Otherwise the source
  first moves into a track of its own (`split_source`, `src/core/state/SplitSource`): keyed by the source when it
  isn't the track key, or else the track keeps it and its other sources move into a track keyed by the first of them.
  The new track takes the old one's status and `addedAt`.
- The track gets `ytmOverride`, loses its failure, and a failed one waits again. When another track already has
  the id, it merges into that one.

A track with an override is never matched again, neither by a sync's search nor from the library.

All tracks shows a row per track, but a playlist shows a row per source it lists (an *entry*), so a merge doesn't
take a row out of a playlist: when it lists several sources of one track, the later ones show as duplicates (ui.md ›
Tracks, sources and entries). `track_entries` (`src/ui/tracks/TrackEntries`) lists every entry in the left panel's
order, which picks the source All tracks shows a track by.

`merge_tracks` (`src/core/state/MergeTracks`) does it in the state. A taken-over `orphaned` or `kept` status becomes
`downloaded` when the survivor is in a playlist. The file work follows from the state the merge leaves (Library ›
Files follow the state), so a state that can't be saved leaves both files.

An override with another id on a track that has a file makes it `pending`, and it downloads again. Its old file stays
until the new one is placed, which then replaces it. An override back to the id the file was made from makes it
`downloaded` again.

**Tracks that leave every playlist.** After a fetch and after `removePlaylist`, a track none of whose sources is in a
playlist anymore is looked at again (`prune_tracks`, `src/core/state/`):
- A track without a `file` is deleted with its sources: there is nothing in the library to decide on.
- A track with a `file` becomes `orphaned` (see SyncEngine › Orphans), also one waiting to download again.

A listed track that is `orphaned` or `kept` becomes `downloaded` again.

**Effective metadata.** Tags and file names use, per field, the first of:
1. `edits`;
2. the track's first Spotify source, because Spotify metadata is the cleanest (separate artists, proper album,
   ISRC);
3. the track's first YouTube Music source.

A field that is empty in one goes on to the next, so a merged track shows its Spotify album before its YouTube Music
source has one. `effective_metadata` (`src/core/state/EffectiveMetadata`) gives the title, artists, album and length
by these rules, the length in the same order; so far there are no `edits`. The track table and the cards show it.

The cover art comes from the first of:
1. the custom cover;
2. the first Spotify source's `coverUrl`;
3. the YouTube Music thumbnail.

### tools.json

```json
{
  "version": 1,
  "checkedAt": "2026-10-22T09:12:00Z",
  "tools": {
    "yt-dlp": { "release": "2026.10.21", "digest": "sha256:..." },
    "ffmpeg": { "release": "n9.0",       "digest": "sha256:..." },
    "deno":   { "release": "v2.5.4",     "digest": "sha256:..." }
  }
}
```

- `tools` has one entry per download, keyed by its name. The ffmpeg entry covers `ffmpeg.exe` and `ffprobe.exe`,
  which come from one archive. `digest` is the downloaded asset's.
- `checkedAt` is when the releases were last looked up for an update, and is left out until then.
- `ToolsStore` (`src/core/tools/`) reads and writes it through `FileOps`, in the same way as `SettingsStore`:
  pretty-printed, unknown keys skipped. A file that doesn't read counts as no file, because the tools can always be
  downloaded again.

## Platform services

| Service          | Implementation                                                                       |
|------------------|--------------------------------------------------------------------------------------|
| `HttpClient`     | WinHTTP (`WinHttpClient`). `get` sends the request headers it is given and returns a body; `post` does the same with a request body; `download` streams into a file and reports the bytes received as they arrive. Automatic proxy, redirects followed, timeouts, and a `User-Agent: WestieLib/<version>` that GitHub's API requires. A non-2xx answer throws `HttpError`, which keeps the status and reads `HTTP <code> from <host>`, a connection problem `Couldn't reach <host>: <WinHTTP's message>`. A `std::stop_token` cancels: its callback closes the request handle, which ends a blocked call, and the call throws `Cancelled`. |
| `ProcessRunner`  | `Win32ProcessRunner`. `run(program, arguments, stop)` starts the program with `CreateProcessW`, without a console window, and returns its exit code, stdout and stderr. Arguments and output are UTF-8; the arguments are quoted by the C runtime's rules. A child inherits only its own pipe ends and NUL as stdin (`PROC_THREAD_ATTRIBUTE_HANDLE_LIST`), so children started at the same time never hold each other's pipes open. It starts suspended, joins a Job Object with `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`, and only then runs. A `std::stop_token` cancels: its callback terminates the job, and the call throws `Cancelled`. Children get `PYTHONUTF8=1` and `PYTHONIOENCODING=utf-8`, but yt-dlp.exe, a frozen Python program, ignores them and writes some lines in the ANSI code page. So each line of output that isn't valid UTF-8 is read in the ANSI code page, and what the core gets is always UTF-8. It returns the whole output at the end, and with an `on_output_line` callback it also hands over each line of stdout as it arrives (without its `\r\n`), on the calling thread, which the Downloader's progress reads. |
| `FileOps`        | Atomic replace: a flushed `.tmp` renamed over the file, or through `ReplaceFileW` when a backup is kept (`replace_file_keeping_backup`). `move_replacing` moves over an existing file (`MoveFileExW` with `MOVEFILE_REPLACE_EXISTING`); `move_new` moves a file to a name that must be free, across volumes too (`MOVEFILE_COPY_ALLOWED`), and answers false when it is taken. Files are deleted outright (`remove_file`), never sent to the Recycle Bin. `exists` tells whether anything has that name. `open_for_append` hands out an `AppendFile`, which appends through a handle opened with `FILE_APPEND_DATA` and shared for reading and writing, so an editor can open the file meanwhile. `list_files` and `remove_file` serve the log's clean-up. |
| `TimeZone`       | `Win32TimeZone`: a `system_clock` time as local time, to the millisecond (`SystemTimeToTzSpecificLocalTime`). The static MinGW build has no time zone database for `std::chrono::current_zone`. |
| `Hasher`         | SHA-256 of a file via BCrypt (`BCryptHasher`), as lowercase hex.                     |
| `ArchiveExtractor` | miniz (`MinizArchiveExtractor`). Extracts the named files from a zip into a folder. miniz opens files by narrow name, so it gets a `FILE*` from `_wfopen`, and writes go through `CreateFileW`. A stop request ends it with `Cancelled`. |
| `TextNormalizer` | `Win32TextNormalizer`, UTF-8 in and out: `NormalizeString` (NFKD), combining marks dropped (`GetStringTypeW`, `C3_NONSPACING`), lowercased with `LCMapStringEx` in the invariant locale. |

Strings inside the core are UTF-8 `std::string`; paths are `std::filesystem::path`. Conversion to UTF-16 happens
inside the platform services.

## Core components

### Settings and StateStore

`SettingsStore` (`src/core/settings/`) loads and saves `settings.json` as a `Settings`, through `FileOps`, and holds the
defaults that `main.cpp` gives it. `main.cpp` loads it before any window shows. A file that doesn't read stops the
app with a message that names the file, rather than writing the defaults over it. Each change is saved at once:
`FileOps::replace_file` writes `settings.json.tmp`, flushes it, and renames it over `settings.json`. The store also
keeps the settings last loaded or saved behind a mutex (`current()`), so the SyncEngine's threads read what the UI
saved last.

`StateStore` (`src/core/state/`) owns `state.json` in memory behind a mutex. Every write
goes to `state.json.tmp` and replaces `state.json` with `ReplaceFileW`, which keeps the previous version as
`state.json.bak`. The SyncEngine saves after every finished track, so an interrupted run resumes where it stopped.

- `main.cpp` makes it before any window shows, and it loads the file then. Without a file the state is empty. A file
  that doesn't read stops the app with a message that names it and points at `state.json.bak`, as for
  `settings.json`: the state holds the user's edits, so it is never overwritten.
- `update(change)` applies a change to a copy, saves the copy, and only then keeps it, so a failed save leaves the
  state as it was.
- The SyncEngine's worker writes the state while the UI reads it. So the store keeps the state as a
  `std::shared_ptr<const State>`: `snapshot()` hands out the current one under a mutex, and a reader keeps it for as
  long as it needs, unchanged. `update` holds a second mutex for its whole run, so changes never interleave, and swaps
  the new state in only after the save.
- So far the state has playlists with `url`, `name`, `addedAt`, `sources`, `lastSync`, `fetchError` and
  `fetchErrorAt`; sources with `title`, `artists`, `album`, `year`, `durationMs`, `explicit`, `coverUrl`, `match` and
  `track`; tracks with `sources`, `ytmId`, `ytmOverride`, `status`, `failure`, `error`, `file` and `addedAt`; and
  `unavailableYtmIds`. Empty
  fields are left out, and a failure or doubt name from a later version reads as none. `isrc` and `edits` arrive with
  their features.
- Earlier versions wrote `ytmId` and `ytmOverride` as `videoId` and `videoOverride`. Those keys still read, and the
  next save writes the new ones.

A named mutex makes WestieLib single-instance, so exactly one process writes the state.

### Log

`Log` (`src/core/log/`) writes what the core does to this run's log file and sends each line to the UI as a `LogLine`.
`main.cpp` makes it right after the event queue, before the workers, so they can log while they stop, and hands it to
every component that logs.

- At start it creates `logs\`, removes all but the 19 newest `.log` files, and opens `logs\<local time>.log`, named
  like `2026-11-12_22-34-12.log`, so a folder of runs sorts by time.
- `write(level, text, subject)` stamps the time, emits `LogLine`, and appends a line to the file. A mutex keeps lines
  whole and in the same order in the file and the events, as the sync, tool, look-up and search workers and the UI
  thread all write. Under it, `write` takes only the event queue's lock and calls back into no component, so a caller
  may hold its own lock (the SyncEngine writes under its queue's).
- Levels, from least to most severe: `Trace` (the tool runs), `Debug` (what helps find a fault: a download's progress,
  searches and their result counts), `Info` (what the app did), `Warning` (what the user may want to look at) and
  `Error` (what failed). The window state saves the pane's level as 0 to 4, from Trace to Error.
- A subject names what a line is about: a playlist key, a track key, or both, and the name to show for it (the
  track's title, else the playlist's name). The name goes with the line, so it outlives a removed playlist.
- A file line is UTF-8 and reads `<time> <LEVEL> <text>`, with ` [<name>; playlist <key>; track <key>]` after it when
  it has a subject, leaving out the parts it doesn't have. The time is local, to the millisecond:
  `2026-11-12 22:34:12.087 ERROR   Couldn't fetch the playlist: HTTP 404 from api-partner.spotify.com [Slow & bluesy;
  playlist spotify:5aX8kq1sTn2Yz9wxxxxxxx]`. A text's further lines follow, indented by 4 spaces, so every line of
  the file starts with a time or with spaces.
- A tool run writes Trace lines with the subject its caller gives (the track it downloads, the playlist it fetches).
  The command line comes as the tool starts, so the lines between it and the tool's end show how long it took. When
  the tool ends, `write_tool_output` writes `yt-dlp exited with code 1` if the exit code isn't 0, then a line
  `yt-dlp: <line>` for each line the tool wrote to stderr. When it failed, its stdout lines follow the same way; a
  success's stdout is what the app reads, as yt-dlp's JSON answers, which run to megabytes, so it isn't written.
- A download writes `Downloaded 0%`, `Downloaded 37%`, … at Debug for each whole percentage yt-dlp reports, which
  come in uneven steps.
- So far it logs the sync (started, each fetch, the end), the tools (installed, updated, failed), the playlist
  look-up, the Choose a match dialog's searches (started and finished at Debug, as below; a failure as a warning), and
  the commands that change the
  state from the UI (a playlist added or removed, a match confirmed, a match chosen). A merge that sends no
  `TracksMerged` writes its line about the track that is left.
- A sync's match writes three Debug lines per search (it started, it finished, its best result), and one line for how
  the track ended, without the searches. A line states what happened; it never tells the user what to do.

  | Line | Level |
  |------|-------|
  | `Searching YouTube Music for "Taylor Swift Patient Zero"`, as a search starts | Debug |
  | `Searched "Taylor Swift Patient Zero": 20 results` (`1 result`, `no results`), as it ends | Debug |
  | `Best result for "Taylor Swift Patient Zero": "Patient Zero" by Taylor Swift (100%)`, the best ranked of that search's results, by the Matcher's ranking; left out without results | Debug |
  | `YouTube Music refused the search for "…": HTTP 403 from music.youtube.com`, for HTTP 403 or 429, or `No answer from music.youtube.com within 1200 ms` (the timeout it had), which the sync tries again (SyncEngine › So far) | Warning |
  | `Search for "…" failed: HTTP 500 from music.youtube.com`, for any other failure; the outcome line follows | Debug |
  | `Matched "Patient Zero" by Taylor Swift (100%)`, with `, and merged into the track with that YouTube Music id` after a merge | Info |
  | `Needs review: "Weekend Luv" by Quinn Lewis (75%), doubts: explicit` | Warning |
  | `No match found` | Error |
  | `Couldn't search YouTube Music: HTTP 403 from music.youtube.com (tried 4 times)`; `(tried …)` only after retries | Error |
  | `YouTube Music doesn't play 1YKAZmwRGj8 here (Video unavailable. This video is not available), so the track is matched again`, from a download (Unavailable ids) | Warning |

  A match from the library writes only its outcome line, with ` from the library` after the match.
- When the file can't be opened or written, the lines still reach the UI, and the first such failure is an Error line
  of its own.

### RemoteConfig

`RemoteConfig` fetches `https://raw.githubusercontent.com/Rheel17/WestieLib/main/config/remote.json` at startup and
caches it as `remote.json`. A compiled-in default is used until the first successful fetch. It carries the Spotify
GraphQL operation name and persisted-query hash:

```json
{
  "version": 1,
  "spotify": {
    "operationName": "fetchPlaylist",
    "sha256Hash": "a65e12194ed5fc443a1cdebed5fabe33ca5b07b987185d63c72483867ad13cb4",
    "pageSize": 100
  }
}
```

Updating this file in the repository updates every installation on its next start.

So far there is no RemoteConfig: `SpotifyPlaylist.cpp` has these three values compiled in.

### ToolManager

ToolManager installs and updates the external tools in `tools\` from their GitHub Releases:

| Tool             | Repository             | Asset                                   | Updated when                  |
|------------------|------------------------|-----------------------------------------|-------------------------------|
| yt-dlp           | `yt-dlp/yt-dlp`        | `yt-dlp.exe`                            | release tag changes           |
| ffmpeg, ffprobe  | `BtbN/FFmpeg-Builds`   | `ffmpeg-nX.Y-latest-win64-gpl-X.Y.zip`, the highest `X.Y` | release branch `X.Y` changes  |
| deno             | `denoland/deno`        | `deno-x86_64-pc-windows-msvc.zip`       | release tag changes           |

`yt-dlp/FFmpeg-Builds` stopped publishing release-branch builds and now only has daily master builds, so ffmpeg
comes from BtbN, the upstream that repository forks. Its `latest` release is rebuilt daily too, which is why the
branch, and not the digest, decides an update.

`choose_download` (`ToolSource`) picks deno's zip by its exact name and names the release by its tag; only
`deno.exe` comes out of it.

Flow per tool:

1. `GET https://api.github.com/repos/<repo>/releases/latest` (`GitHubReleases`, `src/core/github/`), read with glaze.
   `WESTIELIB_GITHUB_API` replaces `https://api.github.com`; the UI checks point it at an address that refuses, so
   they never reach the network.
2. `ToolSource` picks the asset and names the release, and the release is compared with `tools.json`. A missing tool
   has no release, so it always installs.
3. `ToolInstaller` downloads the asset into `tools\.staging\<tool>\download`. The asset's name comes from the network,
   so it never names a file.
4. Verify its SHA-256 against the `digest` GitHub reports for the asset. An asset without one is refused.
5. Extract zip archives (`ArchiveExtractor`) into `tools\.staging\<tool>\unpacked\`. Only the tool's own files come
   out, found by file name wherever they are in the zip (ffmpeg's are under `<build>\bin\`); ffplay and the docs stay
   in.
6. Move the new files into `tools\` (`MoveFileExW` with `MOVEFILE_REPLACE_EXISTING`) while no tool process is running
   (see Tool folder below), then update `tools.json`.

`tools\.staging` is cleared when a run starts and when it ends; a run stopped by the app's close leaves its partial
download there until the next one.

**Tool folder.** `ToolFolder` (`src/core/tools/`) knows the tools folder and each tool's files, and holds a
`std::shared_mutex` per download. Whatever runs a tool holds the shared lock for as long as the process runs
(`lock_for_use`), and `ToolInstaller` holds the exclusive one while it moves the new files in (`lock_for_replace`). So
an update waits for running tools to end, and a tool never starts halfway through an update. `main.cpp` makes it, and
hands it to ToolManager and to the components that run tools.

**yt-dlp.** `YtDlp` (`src/core/tools/`) runs yt-dlp through `ProcessRunner` under `lock_for_use`, always with
`--ignore-config`, so a user's own yt-dlp configuration can't change its output. Without the executable it throws
`ToolMissing`.

ToolManager runs at startup and before the first sync of a session.
- Missing tools are always installed.
- Installed tools are updated only when `autoUpdateTools` is on, or when `checkForUpdates` asks for it.

ToolManager exposes the absolute tool paths to the other components, and which tools are available: installed, and
not being replaced by an update right now. A sync doesn't wait for the tools as a whole. Each step waits only for the
tools it needs (ui.md, Vocabulary › Waiting for tools):

| Step                                   | Needs          |
|----------------------------------------|----------------|
| Playlist look-up, Spotify              | –              |
| Playlist look-up, YouTube Music        | yt-dlp         |
| `SpotifySource` fetch                  | –              |
| `YouTubeMusicSource` fetch             | yt-dlp         |
| Matcher                                | –              |
| `searchCandidates`                     | –              |
| Downloader (audio, thumbnail crop)     | yt-dlp, ffmpeg, deno |

A step whose tool isn't available stays queued: a fetch's `PlaylistFetching` names the tools it waits for, and the
downloads' `DownloadsWaiting` does for all the run's downloads at once. It isn't a failure, and nothing is written to
the state for it. When ToolManager
reports a tool installed, the waiting steps go ahead. So a sync can run while tools install, and a failed tool
install leaves the work waiting, not failed.

- `run(update_installed)` starts a run on a worker thread (`std::jthread`), which `stop()` asks to stop. The
  destructor joins it. The UI starts a run only while none is active.
- A run starts and ends with `ToolsRunChanged`. `run` emits the first one itself, so the UI knows of the run before
  anything else it reports.
- Each step emits a `ToolStatus` (download, state, release, percent, error), so the UI can show
  `Missing`, `Installed`, `Checking`, `Downloading`, `Installing`, `UpToDate` or `Failed`. A run reports the saved
  `checkedAt` as `ToolsChecked`, and a look-up of the releases ends with a new one.
- An exception on the worker would end the process, so the run turns it into a `Failed` status for each tool. A
  failure for one tool fails only that tool, and the run goes on with the next.
- A failed install or update leaves the previous files and `tools.json` entry in place.
- A look-up that went well sets `checkedAt`, even when the install after it fails.
- Closing the app stops the run: the download is cancelled, and the thread is joined before the process ends.

### SelfUpdater

SelfUpdater runs at startup, before ToolManager.

1. `GET https://api.github.com/repos/Rheel17/WestieLib/releases/latest`.
2. Compare the `vX.Y.Z` tag with the version compiled in from CMake's `project(VERSION)`.
3. Download the `WestieLib.exe` asset to `<exe dir>\WestieLib.exe.new` and verify its digest.
4. Rename the running `WestieLib.exe` to `WestieLib.exe.old`, rename `WestieLib.exe.new` to `WestieLib.exe`.
5. Start the new executable and exit.

Every start deletes a leftover `WestieLib.exe.old`. When the executable's folder is read-only, SelfUpdater reports
the available version through an event.

### PlaylistSource

```cpp
struct SourceTrack {
	std::string key;                       // source key: "ytm:<ytmId>" / "spotify:<trackId>"
	std::string title;
	std::vector<std::string> artists;
	std::string album;
	std::optional<std::string> isrc;
	std::optional<std::string> coverUrl;
	std::optional<std::string> ytmId;    // set by YouTubeMusicSource
	std::chrono::milliseconds duration;
};

class PlaylistSource {
public:
	virtual ~PlaylistSource() = default;
	virtual PlaylistInfo fetch(const std::string& url, CancelToken&) = 0;   // name + ordered SourceTracks
};
```

The source type follows from the URL: `music.youtube.com` / `youtube.com` playlist URLs go to
`YouTubeMusicSource`, `open.spotify.com/playlist/...` URLs go to `SpotifySource`.

**Playlist links.** `parse_playlist_link` (`src/core/playlists/`) turns a pasted link into a source type and a
playlist id. It drops spaces around the text; `http://` or `https://` is optional, and host names ignore case.

| Source        | Accepted                                                                                  | Id                         | Canonical `url`                                  |
|---------------|-------------------------------------------------------------------------------------------|----------------------------|--------------------------------------------------|
| YouTube Music | `music.youtube.com`, `youtube.com`, `www.youtube.com`, `m.youtube.com` with `/playlist?…list=<id>…`; `music.youtube.com/browse/VL<id>` | `[A-Za-z0-9_-]+`, except `LM`, `WL` and `LL` (Liked Music, Watch later, Liked videos), which are never public | `https://music.youtube.com/playlist?list=<id>` |
| Spotify       | `open.spotify.com/playlist/<id>`, also after an `/intl-xx/` segment, with any query; the URI `spotify:playlist:<id>` | 22 characters of `[A-Za-z0-9]` | `https://open.spotify.com/playlist/<id>` |

`addPlaylist` stores the playlist under `<source prefix>:<id>` with its canonical `url`, the `name` its look-up found
and `addedAt`, and refuses a key that is already there.

**Playlist look-up.** Before a playlist is added, `PlaylistLookup` (`src/core/playlists/`) finds its name, which also
tells whether it is public. `lookUpPlaylist(link)` runs on a worker thread (`std::jthread`) and answers with
`PlaylistLookedUp`: the playlist key and one of
- `Found`, with the name;
- `Unavailable`: the playlist doesn't exist or is private;
- `ToolMissing`: yt-dlp isn't installed, so the look-up waits for it;
- `Failed`, with the reason, for anything else (no network, an answer that doesn't read).

A new look-up stops the one before, and so does `stop()`; a stopped look-up answers nothing. Per source:

- **YouTube Music**: `yt-dlp --flat-playlist -J -I 0 <canonical url>` lists no entries, only the playlist, and reads
  `title` from the JSON. A missing playlist ends with `ERROR: [youtube:tab] <id>: YouTube said: The playlist does not
  exist.`; an `ERROR:` line that says the playlist doesn't exist or is private means `Unavailable`, and any other
  failure is `Failed` with that line, without its `ERROR: [extractor] <id>:` prefix.
- **Spotify**: `GET https://open.spotify.com/embed/playlist/<id>` and the `__NEXT_DATA__` JSON in it, read with glaze:
  `props.pageProps.state.data.entity.name`. A missing or private playlist still answers 200, with an error page whose
  `pageProps` has no `state`; that means `Unavailable`. `WESTIELIB_SPOTIFY_URL` replaces `https://open.spotify.com`;
  the UI checks point it at a fake server of their own, so they never reach the network.

**YouTubeMusicSource** runs `yt-dlp --flat-playlist -J <url>` and maps each entry to a `SourceTrack` with its
`ytmId`. The album and year of these sources come from yt-dlp's info JSON at download time.

- `fetch_youtube_music_playlist` (`src/core/sources/YouTubeMusicPlaylist.cpp`) does it. A playlist of about 1000
  entries takes yt-dlp some 7 s and 1.2 MB of JSON.
- An entry gives `id`, `title`, `duration` (seconds) and `channel`. The channel is the only artist the flat list
  knows. Songs from an auto-generated channel have ` - Topic` after the artist's name, which is dropped.
- `[Private video]` and `[Deleted video]` entries can't be downloaded, so they are skipped.
- Errors read as in the look-up (`src/core/sources/YtDlpError.cpp`). A playlist that doesn't exist or is private
  fails with "The playlist doesn't exist or is private."

**SpotifySource** reads public Spotify playlists:

1. `GET https://open.spotify.com/embed/playlist/<id>` and parse the `__NEXT_DATA__` JSON. It holds an anonymous
   access token (`props.pageProps.state.settings.session.accessToken`) and its expiry. The page's own `trackList`
   stops at 100 tracks and joins the artists into one string, so the tracks come from the query below. The public Web
   API (`api.spotify.com/v1`) answers this token with HTTP 429.
2. Page through `GET https://api-partner.spotify.com/pathfinder/v1/query` with `operationName`,
   `variables` (`uri`, `offset`, `limit`, and `enableWatchFeedEntrypoint: false`, which the query requires) and
   `extensions` (persisted-query hash from RemoteConfig), sending `Authorization: Bearer <token>` and
   `app-platform: WebPlayer`.
3. Map each item to a `SourceTrack` with title, artists, album, duration, the largest cover URL and the ISRC when
   present.

Each fetch reads a new token from the embed page. A fetch takes seconds and a token lasts most of an hour, so it is
never refreshed.

- `fetch_spotify_playlist` (`src/core/sources/SpotifyPlaylist.cpp`) does it, and `read_spotify_embed_page`
  (`SpotifyEmbedPage.cpp`) reads the page for it and for the look-up. `WESTIELIB_SPOTIFY_API` replaces
  `https://api-partner.spotify.com`; the UI checks point it at the same fake server as `WESTIELIB_SPOTIFY_URL`.
- The answer is `data.playlistV2`, with `content.totalCount` and `content.items`. An item's `itemV2.data` has
  `__typename`, `uri` (`spotify:track:<id>`), `name`, `artists.items[].profile.name`, `albumOfTrack.name`,
  `albumOfTrack.coverArt.sources` and `trackDuration.totalMilliseconds`. It has no ISRC and no year.
- Only items of type `Track` count; episodes and local files are skipped. Pages of 100 are read until `totalCount` or
  an empty page. After each page but the last, the fetch hands the tracks read so far to the SyncEngine, which shows
  them (see SyncEngine › So far).
- A source keeps the title, artists, album, duration and the largest cover URL (`coverUrl`).
- Errors:
  - the embed page's error page, or `playlistV2` of type `NotFound`: "The playlist doesn't exist or is private.";
  - an answer with `errors` (`PersistedQueryNotFound` when the hash is stale): "Spotify said: <message>";
  - any HTTP error as `HttpClient` words it. Spotify's servers answer a page with HTTP 500 now and then, and the same
    request goes through a moment later, so a page that gets a 5xx is asked for once more, a second later; a stop
    request ends that wait at once.

### Matcher

The Matcher turns a Spotify source into a YouTube Music `ytmId`. It runs only for tracks that have no
override and no YouTube Music source whose id is available (see YouTube Music id resolution). For a track whose YouTube
Music sources are all unavailable and that has no Spotify source, it matches the first YouTube Music source the same way
(see Unavailable ids).

A wrong match puts another recording in the library, which is worse than asking the user to confirm a right one. So
the Matcher:
- answers two questions apart: *the same song?* (the title and the artists) and *the same recording?* (the title's
  version words, the length, the album and the explicit flag);
- judges each piece of evidence on its own instead of adding scores up, so a good title can't make up for a wrong
  length;
- fails closed: a title word it can't explain counts as another recording. Its word lists name what can't change what
  you hear, so a word missing from them costs a review, never a wrong match.

1. **Queries.** `"<artists> <title>"`, then, when that finds nothing usable, `"<first artist> <bare title>"`, the
   title without bracketed parts, a ` - ` suffix or a `feat.` tail (skipped when it is the same query). The Spotify
   fetch has no ISRC, so there is no ISRC query.
2. **Search.** YouTube Music's own search, not yt-dlp: yt-dlp's flat search results carry only the id and
   title, and extracting each candidate in full costs about 2 s. `YouTubeMusicSearch::songs`
   (`src/core/matching/YouTubeMusicSearch`) sends `POST <api>/youtubei/v1/search?prettyPrint=false` with
   `context.client` (`WEB_REMIX`, a compiled-in `clientVersion`, `hl: en`), the query, and the songs filter's `params`.
   One answer, about 0.4 to 1.2 s, holds some 20 songs. A refusal (HTTP 403) takes 5 s or more to arrive, so a
   search without an answer in time throws `HttpTimeout` and counts as refused. The time is calibrated on the
   connection (`SearchTimeout`): 4 s until 10 searches have answered, then twice the slowest of the last 50 answers,
   from 1 s to 4 s. Three timeouts in a row, which refusals rarely make, start it again from 4 s. The sync and the
   Choose match dialog share one `YouTubeMusicSearch`, so one calibration. Each `musicResponsiveListItemRenderer` in a
   `musicShelfRenderer` gives `playlistItemData.videoId`, the title (column 0's runs), and column 1's runs, which
   ` • ` splits into the artists (separated by `, ` and ` & ` runs), the album, and the length (`m:ss`). Its `badges`
   hold `MUSIC_EXPLICIT_BADGE` for an explicit song. An answer without a shelf has no results. `WESTIELIB_YTMUSIC_API`
   replaces `https://music.youtube.com`; the UI checks point it at their fake server.
3. **Facts.** Each song brings its title, artists, album, length and explicit flag. A YouTube Music source from a
   playlist has no album and no explicit flag; a stored match keeps those of the result it came from.
4. **Words** (`MatchWords`). `TextNormalizer` lowercases the text and drops accents (NFKD without its marks). Then:
   - apostrophes go; every kind of quote, CJK quote marks too, becomes `"`, which the title parts read; `・`, `、`
     and `。` become spaces; en and em dashes become `-`;
   - a hyphen inside a word joins it (`re-recorded`, `JAY-Z`);
   - `&` and `+` read `and`, `×` reads `x`, and `$` reads `s` (`Ke$ha`, `A$AP Rocky`).

   Words are the runs of letters and digits, with `pt`, `vol`, `ver`, `ft`, `featuring` and `versus` written out
   (`part`, `volume`, `version`, `feat`, `vs`). Names and titles are also compared by their letters alone, so spacing
   doesn't count (`Glasshouse`, `Glass House`; `Funkytown`, `Funky Town`).
5. **Title parts** (`TitleParts`). A title is the song's **name** plus **parts**:
   - the name is the text before the first ` - `, outside brackets;
   - the parts are every bracket, at any depth, and every ` - ` suffix; a bracket inside a suffix belongs to it. A
     part splits further at `;` and before a `feat` or `with` inside it (`Remix; feat. Cardi B`);
   - a part that starts with `from` or `music from` names the work the song comes from (`From "Frozen"`, `Music from
     the HBO Original Series`). It is left out, with the brackets it holds; a ` - ` suffix of that kind goes with the
     brackets in its quotes (`- From "Tell Me Lies (Season 2)"`), not with one after them (`- from the series Arcane
     League of Legends (Baby Tate Remix)`);
   - in the name, a `feat` tail is a part too. A `with` tail (`Retrograde with Glass House`) and a `Name: ` lead
     (`Einaudi: Experience`) stay aside: they leave the name when they name credited people (below), and are part of
     it otherwise (`Dance With Me`, `Swan Lake, Op. 20, Act 2: No. 10`).
6. **Credits** (`Credits`). A part is a credit when it holds only names and connector words (`feat`, `with`, `x`,
   `vs`, `and`, `of`, `by`, `prod`, `produced`, `the`, `cover`, and `con`, `y`, `e`, `et`, `avec`, `mit`, `und`, `cu`,
   `met`), with at least one name. A credit led by `feat` or `with` also counts when it starts with a known name, or a
   known name starts with it (`feat. Count Basie And His Orchestra`, `feat. Gary Lightbody of Snow Patrol`).
   An artist entry that holds several names knows each of them too, split at commas and at connectors such as `&`
   (`Mike Griffin & The Unknown Blues Band`, `Kina Grannis, Kurt Hugo Schneider`). Spotify's metadata is the
   reference, so which names are known depends on the side:
   - the candidate's credits must name someone the Spotify track credits: one of its artists, or a name its title
     credits. So `Levitating (feat. DaBaby)` is another recording than Spotify's solo `Levitating`, even when YouTube
     Music lists DaBaby among the artists;
   - the Spotify title's credits may name anyone either song credits, because YouTube Music often leaves guests out.

   `cover` is a connector because the artists check verifies who performs. `remix` isn't: a remixer is listed as one
   more artist, so the artists check can't tell remixes apart. A lead leaves the name when its words are a known name,
   or that name's last words (a surname); a `with` tail leaves it when it is a credit.
7. **Version words.** The words of a title's parts, without its credits, and without the words that don't change what
   you hear, or whose change the length shows:
   - edition and master: `remaster`, `remastered`, `master`, `reissue`, `mono`, `stereo`, `digital`, `digitally`,
     `explicit`;
   - release and cut: `version`, `original`, `album`, `single`, `edit`, `radio`, `extended`, `mix`, `short`, `long`,
     `full`, `deluxe`, `edition`, `bonus`, `track`, `soundtrack`;
   - upload: `official`, `video`, `audio`, `lyric`, `lyrics`, `visualizer`, `hd`;
   - years (1900 to 2099), and any number in a part that holds one of the words above (`Digitally Remastered 95`,
     `12" Version`).

   Everything else is a version word: `live`, `remix` and the remixer, `acoustic`, `korean`, `taylors`, `classic`,
   `moonlit witch`, `kya yehi pyaar hai`, `vf`, `cis`, `cappella`, a guest no one credits. Two titles have the same
   version when their sets of version words are equal, or when those words, joined in order, have the same letters
   (`StayLoose Remix`, `Stay Loose Remix`).
8. **Evidence** (`MatchEvidence`), each piece judged on its own:

   | Evidence | Levels |
   |----------|--------|
   | Name | **Same**: the same letters. **Close**: one word more or less, with 2 or more left, or 1 letter off from 8 letters on and 2 from 16 (`It Serves You Right…`, `It Serve You Right…`). **Other** |
   | Artists | Against the candidate's artists and, for an entry that holds several, the names between its commas (`Kina Grannis, Kurt Hugo Schneider`). **Same**: the first Spotify artist has the letters of a candidate artist, a leading `the` aside. **Likely**: one of the two names starts the other, by words or from 4 letters (`Prince`, `Prince & The Revolution`; `Tora`, `ToraMusic`; `Lizzo`, `Lizzo Music`). **Doubtful**: one name sits inside the other, from 4 letters (`Ms. Lauryn Hill`), or only another Spotify artist matches (`Kygo, Parson James`, `Parson James`). **Other** |
   | Version | the same, or not (above) |
   | Length | **Same**: within 2 s. **Close**: within 5 s. **Near**: under 10 s. **Off**. **Unknown** when a side has none |
   | Album | **Same**: the same letters in the album's name, its parts aside (`Night Visions (Deluxe)`). **Other**. **Unknown** when a side has none |
   | Explicit | the same, not, or unknown |

9. **Verdict** (`judge_match`, `src/core/matching/MatchJudgement`):
   - **Reject**, not a match at all:
     - another song: the name is Other;
     - another artist: the artists are Other. Unless the name is Same, the version the same, and the length and the
       album Same: then it is a review with an artist doubt, an alias such as `Cass Elliot` and `Mama Cass`;
     - another recording: the version differs and the length isn't Same (a live version, another remix).
   - **Review** when anything is in doubt (`MatchDoubt`, `src/core/state/MatchDoubt.h`):
     - `title`: the name is Close;
     - `artist`: the artists are Doubtful, unless the length and the album are both Same; or Likely, with neither a
       Same length nor a Same album; or Other, rescued as above;
     - `version`: the version differs, while the length is Same;
     - `length`: the length is neither Same nor Close, nor Near with a Same album, so 10 s off is always in doubt;
     - `explicit`: the explicit flags differ.
   - **Accept** when nothing is in doubt.

   The **confidence** is stored with the match and shown as a percentage. It follows from the verdict, so the review
   threshold (0.85) falls between Accept and Review:

   | Verdict | Confidence |
   |---------|------------|
   | Accept | 1, or 0.9 when the length or the artists aren't Same |
   | Review | 0.75 with one doubt, 0.65 with two, 0.55 with more |
   | Reject | 0.3 for another recording, 0.15 for another artist, 0.05 for another song |

   **Ranking** (`match_rank`, `src/core/matching/MatchRank`) orders the candidates by, in turn: the verdict (Accept,
   Review, Reject; among rejects another recording, then another artist, then another song); fewer doubts; stronger
   artist evidence, since another artist may be a cover or a reupload while a length a few seconds off is often the
   same recording; a closer length class; a YouTube Music id that a track already has, so the same song from both sources tends
   to end up as one track; a Same album; the same explicit flag; stronger name evidence; a smaller length difference;
   and last YouTube Music's order.
10. **Choose.** The searches run in turn, their candidates pooled (each id once), until one gives an Accept. Then
    the pool's best ranked candidate decides:
    - Accept or Review: the source's `match` stores the YouTube Music id, the confidence and doubts, and the candidate's
      title, artists, album, length and explicit flag. A Review makes the track need review (see YouTube Music id resolution)
      until the match is confirmed or an override is set. If an accepted id already belongs to another track,
      the two tracks merge instead of downloading.
    - Reject, or no results: the track fails with `failure: noMatch` and the searched queries in `error`. A search
      that fails (no network, an HTTP error) fails the track with `failure: search` and the reason in `error`. Either
      way the next sync tries again. YouTube Music answers some searches with HTTP 403 (or 429) now and then, while
      the ones around them go through, so a sync tries those again first (SyncEngine › So far), and those that get no
      answer in time too.

`Matcher` (`src/core/matching/`) runs the queries. `MatchWords`, `TitleParts`, `Credits`, `NameMatch`, `SongFacts`,
`MatchEvidence`, `MatchJudgement` and `MatchRank` are pure logic, which the unit checks hold against real cases
(`docs/testing.md`).

For the track card's list of doubts, `version_difference` (`MatchEvidence`) gives the version words only one of two
titles has, as that title writes them and in its order, each once: `Korean` for `Love U Like That - Korean Version`
against `Love U Like That`, `feat.` and `DaBaby` for `Levitating (feat. DaBaby)` against `Levitating`. It takes the
titles and artists the match stores, so the UI never parses a title.

**The library first.** YouTube Music keeps one recording under several ids (a single, an album, a re-release),
and a playlist can swap one for another. So a Spotify track and the YouTube Music playlist entry of the same song
can end up with different ids, and never merge. Before it searches, a sync compares every Spotify track in the
library that has no YouTube Music source a playlist lists (and isn't Removed or Kept) with the YouTube Music sources
the playlists list (`library_matches`, `src/core/matching/LibraryMatches`): the same judgement, against the sources
whose name has the same letters, with or without a `with` tail (so `Funkytown` finds `Funky Town`). The best ranked
Accept becomes the Spotify source's match, and the track merges into that source's track (`apply_match`): the
playlist owner's choice wins over a search result. A Review leaves the track to the search.

It looks at the whole library, not only at the playlists the sync fetched, so the order in which playlists are added
or synced doesn't matter: a Spotify track the search matched earlier joins a YouTube Music playlist's entry on the
first sync after one lists the song, and a Spotify track that comes after the entry merges before it is searched for.
A downloaded track merges by the Merging rules, files and all.

A stored match keeps the confidence and doubts it got; a later change to the rules applies to new matches only. Until
release, playlists are added again to match their tracks anew.

**Candidate search.** `searchCandidates(source, query)` serves the UI's Choose a match dialog, for a Spotify source.
`CandidateSearch` (`src/core/matching/CandidateSearch`) runs it on its own worker thread, outside any sync run; a new search
stops the one before, and a stopped one answers nothing. It answers with `CandidateSearchResults`: the request's number,
the candidates, and the error of a search that failed. Searching needs no tool: it is the Matcher's own search
(`YouTubeMusicSearch::songs`), for the query, or without one for each of the Matcher's queries (`match_queries`,
`src/core/matching/MatchQueries`).

`match_candidates` (`src/core/matching/MatchCandidates`) makes the list from a fresh snapshot, each id once, by
rank (`match_rank` of its judgement against the source), ties in this order:
1. the source's match;
2. the YouTube Music sources the playlists list (`listed_sources`, `src/core/state/ListedSources`) that the
   judgement doesn't reject;
3. the search results.

Each candidate has the YouTube Music id, title, artists, album (none for a library source), length, score (the judgement's
confidence), whether it is the source's track's id, and the track that has it otherwise (`track_with_ytm_id`). A
failed search still answers with the first two.

**YouTube Music links.** `parse_ytm_link` (`src/core/matching/YouTubeMusicLink`) turns a pasted link into a YouTube Music id. It drops
spaces around the text; `http://` or `https://` is optional, and host names ignore case.

| Accepted | Example |
|----------|---------|
| `music.youtube.com`, `youtube.com`, `www.youtube.com`, `m.youtube.com` with `/watch?…v=<id>…` | `https://music.youtube.com/watch?v=5aM1xdk8lRw&si=…` |
| the same hosts with `/shorts/<id>` | `youtube.com/shorts/5aM1xdk8lRw` |
| `youtu.be/<id>`, with any query | `https://youtu.be/5aM1xdk8lRw?si=…` |
| the id alone | `5aM1xdk8lRw` |

An id is 11 characters of `[A-Za-z0-9_-]`.

The UI lists the doubts in the track card's Status section, and the verdict in Choose a match's `Match`
column (`ui.md`).

### Downloader

The Downloader produces one tagged `.opus` or `.flac` file in `work\<track key>\`:

```
yt-dlp -f "bestaudio[acodec=opus]/bestaudio" -x --audio-format <format>
       --write-info-json --write-thumbnail --convert-thumbnails jpg
       --ffmpeg-location <tools> --js-runtimes deno:<tools>\deno.exe
       --newline --progress-template "download:[progress] %(progress._percent_str)s"
       --no-playlist -o "<work>\%(id)s.%(ext)s"
       https://music.youtube.com/watch?v=<ytmId>
```

- **Format.** `<format>` comes from `settings.format` at download time.
  - `opus` (the default): Opus sources are remuxed into `.opus` without re-encoding, and AAC sources are
    transcoded to Opus.
  - `flac`: the audio is transcoded to FLAC. The source is lossy, so FLAC changes the container but not the
    quality.

  A format change applies to new downloads only. Existing files keep their format, so a library can mix both.
- Progress lines from `--progress-template` become `TrackProgress` events. `download_percent`
  (`src/core/download/DownloadProgress`) reads a line: `[progress] ` and a percentage such as ` 45.3%`, rounded down to
  a whole percent from 0 to 100; any other line is no progress. The template's `download:` only says which progress it
  is for, so yt-dlp doesn't print it; `[progress]` is what marks the line. Color codes, which yt-dlp writes when it
  takes the pipe for a terminal, are left out. yt-dlp prints a line per update, so the Downloader
  sends an event only when the whole percent changes.
- yt-dlp runs under the shared locks of yt-dlp, ffmpeg and deno (Tool folder), as it runs the other two. A missing
  one throws `ToolMissing` for it, before anything starts.
- A non-zero exit code throws `DownloadFailed`, whose text is the `ERROR:` lines of stderr, or its last two lines when
  it has none.
- yt-dlp names the audio file after the id and the format (`<ytmId>.opus`, `<ytmId>.flac`), the info JSON
  `<ytmId>.info.json`, and the thumbnail `<ytmId>.jpg`. A run that exits with 0 without the audio file fails too.
- **Cover art.** The first of these that applies:
  1. a custom cover from `edits` (`covers\<track key>.jpg`);
  2. when the track has a Spotify source, its Spotify image (square, 640 px), fetched with `HttpClient` into
     `cover.jpg` in the work folder;
  3. the YouTube Music thumbnail, center-cropped to a square with `ffmpeg -vf "crop=min(iw\,ih):min(iw\,ih)"` into
     `cover.jpg`.

  No cover is fetched when `embedCover` is off. A cover that fails is a Warning line, and the file goes without one
  (`src/core/download/CoverArt`). So far there are no custom covers.
- **Metadata.** The info JSON fills the `album` and `year` (`release_year`) of the track's YouTube Music sources,
  which a fetch never sets. The tags then use the effective metadata.

### Tagger

The Tagger writes Vorbis comments with TagLib. Both Opus and FLAC files carry Vorbis comments. It uses the
effective metadata, so `edits` win over source values:

| Field                    | Value                                                         |
|--------------------------|---------------------------------------------------------------|
| `TITLE`                  | title                                                         |
| `ARTIST`                 | artists joined with `, `                                      |
| `ALBUM`                  | album                                                         |
| `DATE`                   | `year`                                                        |
| `ISRC`                   | ISRC, when known                                              |
| `WESTIELIB_SOURCE`       | every source key of the track, one value each, track key first |
| `WESTIELIB_YTMID`        | YouTube Music id the audio came from                          |

**Cover art** is the front cover (picture type 3), as a JPEG. It is written only when `embedCover` is on:
- in Opus files, as `METADATA_BLOCK_PICTURE`;
- in FLAC files, as a native `PICTURE` block.

`WESTIELIB_SOURCE` and `WESTIELIB_YTMID` make every file self-identifying, which powers state rebuilds.

The Tagger also re-tags finished library files in place (`update_tags`), after `editTrack` and after a merge; that
keeps the pictures, and the album and date the new tags leave empty. `read_cover` reads the
front cover back out of a file, for the track card; it is empty when the file has none or doesn't read.

`Tagger` (`src/core/tagging/`) replaces the file's own Vorbis comments and pictures with these, so the tags yt-dlp's
ffmpeg left from the video don't stay. It opens the file by its wide name, and a file TagLib can't read or save
throws.

### Library

The Library places finished files and handles the library folder as a whole.

- **File names.** `<ARTIST> - <TITLE>.<opus|flac>`, built from the effective metadata. Characters `<>:"/\|?*` and
  control characters become `_`, trailing dots and spaces are trimmed, reserved device names (`CON`, `NUL`, `COM1`, ...)
  get a `_` suffix, and names are capped at 200 characters. A name that already belongs to another track, or to a file
  in the folder, compared without case, gets ` (2)`, ` (3)`, ... appended. Without a title the name is the id
  (`library_file_name`, `src/core/library/LibraryFileName`).
- **Windows names.** `windows_name_problem` (`src/core/library/WindowsNames`) tells why a file or folder name can't be
  used as it is: it ends in a space or a period, which Windows drops, or it is a reserved device name, with or without
  an extension. The file names above use it, and so does the library folder field (`ui.md`), for each folder of the
  path, so the folder the app creates is the one `settings.json` names.
- **Placement.** The library folder is created when it is missing. The tagged file moves from `work\` into the library
  with `FileOps::move_new`, so the library only ever contains complete files, and a name another file took meanwhile
  moves on to the next number.
- **Files follow the state.** `Library` (`src/core/library/Library`) makes every change that can merge, drop or
  re-tag a track: it saves the change through `StateStore::update`, and then brings the files in line with it. One
  lock covers both, so the file work of two changes never interleaves, and a download's new `file` is stored under it
  too. `file_changes` (`src/core/library/FileChanges`, pure) compares the state before and after the change:
  - A `file` some track had before and none has after is deleted, unless a track has it again by then.
  - A file a track has before and after (the same track, or the survivor of a merge) is re-tagged when the tags it
    should have (`track_tags`: the effective metadata, the source keys and `ytmId`) differ. A file that is new
    after the change was just tagged by its download.
  - When the change gives such a file another stem (`library_file_stem`), it is renamed by the name rules, and its
    `file` follows in a second save.

  Re-tagging (`update_tags`, Tagger) keeps the file's cover, and the album and year when the state doesn't know them:
  a Spotify track's come from the info JSON. A file that can't be deleted, re-tagged or renamed is a Warning line, and
  the state stays as it was saved. So the rules hold for merges, overrides, fetches that change a source's values,
  removals, and later for `editTrack`.
- **Removal.** Files are deleted through `FileOps`, not sent to the Recycle Bin.
- **Relocation.** `relocateLibrary(newPath)` updates `libraryPath`; all `file` entries are relative, so they stay valid.
- **Rebuild.** `rebuildState()` scans the library folder for `.opus` and `.flac` files. From each one it reads
  `WESTIELIB_SOURCE` / `WESTIELIB_YTMID` and the regular tags, and recreates the track and its sources.
  - The file tags become the source values, so `edits` are not restored.
  - Playlist membership is restored on the next sync.

### SyncEngine

The SyncEngine coordinates a sync run on a coordinator thread with a pool of `concurrency` worker threads.

1. **Fetch.** For each selected playlist, the matching `PlaylistSource` returns the ordered source list.
   - On success, the playlist's `sources` and `name` in the state are replaced with it, and `fetchError` is
     cleared. The engine emits `PlaylistFetched`.
   - A new source becomes a new `pending` track with `addedAt`. The exception is a new YouTube Music source
     whose id already belongs to a track: it joins that track (see Merging).
   - On failure, the playlist gets `fetchError` / `fetchErrorAt` and keeps its previous source list, plus the pages
     a Spotify fetch read before it failed, so none of its tracks become orphans. The engine emits
     `PlaylistFetchFailed`, and the run continues with the other playlists.
2. **Orphans.** `downloaded` tracks with no source in any playlist become `orphaned`. The UI finds them in the
   state, so it also shows the ones an earlier run or `removePlaylist` left, and no event is needed.
3. **Queue.** The `pending` and `failed` tracks of the playlists in the run are queued in playlist order. Workers start tracks
   `delayBetweenTracksSeconds` apart.
4. **Per track (worker).**
   1. Resolve the YouTube Music id (see YouTube Music id resolution). This step takes a lock on the id, so two
      tracks that resolve to the same id in one run merge instead of downloading it twice.
   2. If the id already belongs to another track, merge, emit `TracksMerged`, and stop here.
   3. Downloader, then Tagger, then Library placement.
   4. Update the track (`downloaded`, `file`, `ytmId`) and the source's `match`, or mark the track `failed`
      with a `failure`:
      - `noMatch`, with the searched queries as `error`;
      - `download`, with the last lines of tool output as `error`.
   5. Save the state, clean `work\<track key>\`, emit `TrackStatusChanged`.
5. **Finish.** Emit `SyncFinished` with counts per status.

**Retry.** `retry(trackKeys)` sets the tracks to `pending` and queues them without a fetch step. While a run is
active, they join its queue. Otherwise they start a run of their own.
- `retry` takes the failed ones among the tracks it is given, clears their failure in one save, and logs `Retrying 2
  tracks`. A track whose match failed is matched again, and one whose download failed downloads again.
- A run of its own emits `SyncQueued` without playlists, and matches and downloads only those tracks. A `sync` during
  it adds the tracks of the playlists it queues, as an update does.
- During a run, the tracks join its queues even when the run has already tried them.

**Removal decisions** arrive through `resolveRemovals` at any time during or after a run (`resolve_removals`,
`src/core/library/ResolveRemovals`, through `Library`, in one save).
- `remove` deletes the track and its sources, and so its file. It applies to `orphaned` and `kept` tracks.
- `keep` sets an `orphaned` track's status to `kept`.
- A track in a playlist again, or gone, is left alone. The log gets `Kept the file` or `Deleted "<file>"` per track.

A `kept` or `orphaned` track that one of its sources brings back into a playlist returns to `downloaded`.

**Cancellation** sets the run's `CancelToken` and terminates the Job Object. In-flight tracks return to `pending`
and their work folders are cleaned.

**So far** (`src/core/sync/`), a run fetches, YouTube Music and Spotify playlists alike, then matches, then
downloads. Its changes to tracks go through `Library`, so files follow merges and fetches.
- `sync(keys)` queues the playlists that aren't queued yet and emits `SyncQueued`. Without a run it starts one on a
  worker thread (`std::jthread`); during a run they join its queue. The queue is behind a mutex, so a run that finds
  it empty and ends can't miss a `sync` that comes at the same moment.
- Per playlist the run emits `PlaylistFetching` and fetches. `apply_fetch` (`ApplyFetch.cpp`) puts the result in
  the state: the playlist's `name` and `sources`, `lastSync`, no `fetchError`, the source values from the playlist,
  a `pending` track for each new source (keyed like the source), then `prune_tracks`. A new YouTube Music source
  whose id a track already has (a Spotify track matched to it) joins that track instead. A playlist removed during
  the fetch is left alone. Then the run emits `PlaylistFetched`.
- A Spotify fetch also hands over the tracks read so far after every page but the last. `apply_fetch_page` puts them
  in the state: the playlist's `name`, and as its `sources` the ones read so far followed by its earlier sources that
  haven't come back yet, with the source values and new `pending` tracks as above. It leaves `lastSync`, `fetchError`
  and the pruning to the end, so no track leaves a playlist halfway. Then the run emits `PlaylistPageFetched`.
- A failed fetch stores `fetchError` / `fetchErrorAt` and emits `PlaylistFetchFailed`; the run goes on. The pages a
  Spotify fetch read before it failed stay, beside the playlist's earlier sources.
- Without yt-dlp, a YouTube Music fetch emits `PlaylistFetching` with `waitingFor`, and waits until ToolManager installs
  it (`ToolFolder::wait_until_installed`, which `ToolInstaller` signals after moving the files in).
- A run matches and downloads only the tracks of the playlists it syncs, and the ones `retry` gives it. Updating one
  playlist leaves the tracks that wait in another alone, also those the two share; a shared track counts in the
  playlists of the run only (`TracksQueued`, `DownloadsQueued`). Merging what the library matches (below) still looks at
  the whole library.
- Once no playlist is left to fetch, the run first merges what the library matches (Matcher › The library first), in
  one save, without events of their own: the next refresh shows them. Then it matches. `tracks_to_match`
  (`TracksToMatch.cpp`) lists the tracks that are `pending` or `failed`, have no override and no YouTube Music source
  whose id is available, and whose source to match (its first Spotify source, else its first YouTube Music source; see
  Unavailable ids) has no match, in playlist order (playlists by `addedAt`, then their sources), each
  once, each with the playlists that list it. The run emits `TracksQueued` with the ones it hasn't queued yet, and
  matches them one at a time, saving after each. It emits `TrackMatching` as it starts each try, so the UI knows whose
  playlists are matching right now:
  - `apply_match` (`ApplyMatch.cpp`) stores the match, clears a failure (a failed track waits again), and merges the
    track into the one that already has that id (`track_with_ytm_id`, `merge_tracks`), unless the match needs
    review. The run emits `TracksMerged`, or `TrackStatusChanged`.
- `confirm_match` (`src/core/matching/ConfirmMatch`) serves `confirmMatch` on the UI thread: it sets `confirmed` and
  merges the track as `apply_match` would have. `set_ytm_override` serves `setYtmOverride` there too. A match
  that a run makes meanwhile for a track that got an override is dropped (`apply_match` leaves such a track alone).
  - A failed match stores `failed` with `noMatch` or `search` and the error, and emits `TrackStatusChanged`.
  - A track that no longer needs a match by its turn (merged, or gone with its playlist) is skipped, with a
    `TrackStatusChanged` so the UI's counts still end it.
  - A search that YouTube Music answers with HTTP 403 or 429, or doesn't answer in time (Matcher › Search), doesn't fail the track at once. It goes back into the queue
    10 tracks further on (`RetryGap`), or last when fewer are left, and waits at least 10 s (`RetryDelay`) from then;
    nothing is stored and no event is sent. The tracks in between go ahead. When only waiting tracks are left, the run
    sleeps until the first is due; a playlist queued meanwhile, or a cancel, ends the sleep. A track is tried 4 times
    (`MatchTries`); the fourth failure fails it with `search`. Any other failed search fails the track at once. Else a
    track is tried once a run.
  - A playlist queued meanwhile is fetched before the next track, and the tracks it brings join the queue.
- Once the match queue is empty, the run queues the downloads. `tracks_to_download` (`TracksToDownload.cpp`) lists
  the tracks that are `pending`, or `failed` with `download`, have a YouTube Music id (`resolved_ytm_id`) and don't need
  review, in playlist order, each once, each with the playlists that list it. The run emits `DownloadsQueued` with the
  ones it hasn't queued yet, and hands them to its download pool (`DownloadPool`, `src/core/sync/`):
  - `concurrency` worker threads, read from the settings as the pool starts, take tracks in order and start them
    `delayBetweenTracksSeconds` apart. A track whose id another worker is downloading waits until that one ends.
  - Meanwhile the run's own thread fetches the playlists queued since, and matches their tracks; their downloads join
    the pool's queue. The run ends once nothing is left to fetch, match or download.
  - Per track (`TrackDownload`): a snapshot shows whether it still qualifies, else it is skipped with a
    `TrackStatusChanged`. It emits `TrackDownloading`, clears `work\<track key>\` (`:` becomes `_`), and runs the
    Downloader, the cover, the Tagger and the placement, with the settings of that moment. Then `apply_download`
    (`ApplyDownload.cpp`) stores `downloaded`, `ytmId` and `file`, clears the failure, and fills the YouTube Music
    sources' album and year from the info JSON, through `Library`, which deletes a file the new one replaces. A new
    file whose name is that old file's moves over it (`FileOps::move_replacing`), so a download again keeps the name.
    A track that left the state meanwhile (its playlist was removed) has its new file deleted. The work folder is removed, and the run emits `TrackStatusChanged` and logs `Downloaded as
    "<file>"`.
  - A failed download, tag or placement stores `failed` with `download` and the error, logs `Couldn't download:
    <error>`, and emits `TrackStatusChanged`; the next sync tries again.
  - A download that fails as unavailable (Unavailable ids) instead adds the id to `unavailableYtmIds`, removes the
    match that gave it, and leaves the track `pending` without a failure, in one save (`apply_unavailable`,
    `ApplyDownload.cpp`). It logs the Warning line (Logging) and emits `TrackStatusChanged`, and the track joins the
    run's match queue as a playlist's new tracks do. An override's id is the user's choice, so an unavailable override
    fails as an ordinary download, and its id doesn't join the set.
  - `hold_downloads()` stops the pool from starting tracks while the UI's Choose a match dialog is open, since YouTube
    Music doesn't answer searches while yt-dlp downloads. It returns a `DownloadHold`, and the pool goes on when that
    goes. The downloads under way go on, and a pool that starts during the hold starts held. Nothing is logged or sent.
  - A missing tool stops the pool from starting tracks: the one that found it missing goes back to the front of the
    queue, the run emits `DownloadsWaiting` with the tools, and the log gets a Warning `Waiting for ffmpeg to install`.
    Nothing is written to the state. When `ToolFolder` reports the install, `DownloadsWaiting` without tools follows, and
    the tracks go on.
- `cancel()` and `stop()` end the run. The playlist being fetched keeps its previous state, apart from the pages a
  Spotify fetch already read (as for a failure); a track being matched or downloaded stays as it was, and a download's
  work folder is removed; the queues are dropped, and nothing more is written. A run starts by removing what an
  earlier one left in `work\`.
- Every run ends with `SyncFinished`, cancelled or not.

The UI calls a run an *update*, and its first step for a playlist, the fetch, *syncing* (`ui.md`, Vocabulary). The
log uses the UI's words: `Update started: 2 playlists`, `Added 1 playlist to the update`, `Update finished`, `Update
cancelled`.

## Core API

### Commands (UI layer to core)

| Command                                  | Effect                                                         |
|------------------------------------------|----------------------------------------------------------------|
| `lookUpPlaylist(link)`                   | Finds a playlist's name before it is added; answers with `PlaylistLookedUp` |
| `addPlaylist(url, name)`                 | Adds a playlist with the name its look-up found; the source type follows from the URL |
| `removePlaylist(id)`                     | Removes a playlist; tracks left with no source in a playlist are deleted, or become `orphaned` when they have a file |
| `sync(playlistIds)`                      | Starts a sync run                                              |
| `cancel()`                               | Cancels the running sync                                       |
| `retry(trackKeys)`                       | Queues the given tracks again without fetching playlists       |
| `resolveRemovals({trackKey: keep/remove})` | Applies removal decisions                                    |
| `setYtmOverride(sourceKey, ytmId)`       | Sets `ytmOverride` on the source's track, or on a track of its own (see Merging), and re-downloads it, or merges it into the track that already has that id |
| `confirmMatch(trackKey)`                 | Sets `confirmed` on the match the track's id comes from, then merges the track into one that already has that id |
| `searchCandidates(sourceKey, query?)`    | Lists candidates for a Spotify source, searching YouTube Music's songs; answers with `CandidateSearchResults` |
| `editTrack(trackKey, edits, coverImage?)` | Stores `edits` and an optional custom cover. Re-tags the file, renames it when artists or title changed, emits `TrackUpdated` |
| `setSettings(settings)`                  | Saves `settings.json`; applies to the next run                 |
| `relocateLibrary(path)`                  | Points the state at a moved library folder                     |
| `rebuildState()`                         | Rebuilds tracks and sources from file tags                     |
| `checkForUpdates()`                      | Runs SelfUpdater and ToolManager, updating tools even when `autoUpdateTools` is off |

### Events (core to UI layer)

| Event                          | Payload                                         |
|--------------------------------|-------------------------------------------------|
| `SyncStarted` / `SyncFinished` | run summary                                     |
| `SyncQueued`                   | so far: playlist keys added to the run's queue (it replaces `SyncStarted`) |
| `PlaylistFetching`             | playlist key, the tools its fetch waits for     |
| `PlaylistLookedUp`             | playlist key, outcome, name or error            |
| `PlaylistPageFetched`          | so far: playlist key, after each page of a Spotify fetch but the last |
| `PlaylistFetched`              | playlist id, name, track count                  |
| `PlaylistFetchFailed`          | playlist id, error                              |
| `TracksQueued`                 | so far: the tracks the run will match, each with the keys of the playlists that list it |
| `TrackMatching`                | so far: track key, as the run starts matching it (again after a refused search) |
| `TrackStatusChanged`           | track key, status, failure, error                 |
| `DownloadsWaiting`             | so far: the tools the run's downloads wait for, none once they go on |
| `DownloadsQueued`              | so far: the tracks the run will download, each with the keys of the playlists that list it |
| `TrackDownloading`             | so far: track key, as a worker starts downloading it |
| `TrackProgress`                | track key, percent, as it changes by a whole percent |

| `TrackUpdated`                 | track key (tags, cover or file changed)         |
| `TracksMerged`                 | deleted track key, surviving track key          |
| `CandidateSearchResults`           | request number, candidates, error               |
| `ToolsRunChanged`              | whether a ToolManager run is going              |
| `ToolStatus`                   | tool, state, installed release, percent, error  |
| `ToolsChecked`                 | when the releases were last looked up           |
| `AppUpdate`                    | available version, update state                 |
| `LogLine`                      | time, level, text, subject (optional playlist key and track key, and the name to show) |

Events are emitted from any thread through an `EventSink` interface, as a `CoreEvent` (a `std::variant` of the event
structs). The Win32 implementation, `CoreEventQueue` (`src/ui/events/`), appends each event to a queue behind a
mutex. When the queue was empty, it posts one message without a payload to the main window, which then takes every
queued event at once. The queue owns the events, so nothing leaks when the window is gone before it takes them.
The window applies the whole batch first and only then shows the playlists, the tracks and the new log lines, once.
A burst of events then costs one update, so the window keeps up with a fast sync instead of falling behind it.

## Startup sequence

1. Acquire the single-instance mutex.
2. Delete a leftover `WestieLib.exe.old`.
3. Load `settings.json`, `state.json`, `remote.json`, `tools.json`.
4. SelfUpdater check (may restart the process).
5. RemoteConfig fetch.
6. ToolManager installs missing tools and, with `autoUpdateTools`, updates the others.
7. Ready for commands.

## Build

- CMake, Ninja, MSYS2 MinGW-w64, C++26, Unicode entry point (`wWinMain`, `-municode`).
- Linked statically (`-static`), producing one self-contained `WestieLib.exe`.
- Libraries: TagLib, glaze, miniz.
- Windows libraries: `winhttp`, `bcrypt`, `ole32`, `shell32`, `gdiplus`.
- The version comes from `project(VERSION)` and is compiled into a generated header and the executable's
  `VERSIONINFO` resource.
- Releases are published on https://github.com/Rheel17/WestieLib as `vX.Y.Z` tags with `WestieLib.exe` attached.

## Source layout

```
src/
    core/           the logic, in namespace westie::core, one folder per area:
        events/     CoreEvent, EventSink
        github/     the latest release of a repository, from GitHub's API
        json/       what the JSON files share: write options, ISO 8601 times, UTF-8 paths
        log/        Log, LogLine and the log's text formats
        matching/   the Matcher: YouTube Music's search, title words and parts, credits, evidence, verdict and rank
        playlists/  playlist links, the playlist look-up, addPlaylist
        settings/   Settings, SettingsStore
        sources/    SourceType, each source's playlist name, the Spotify embed page, the YouTube Music and Spotify fetches
        state/      State, StateStore, prune_tracks
        sync/       SyncEngine, apply_fetch and the sync events
        tools/      ToolManager, ToolInstaller, ToolSource, ToolsStore, ToolFolder, YtDlp and the tool types
                    later areas get folders of their own: SelfUpdater, RemoteConfig
        download/   the Downloader: yt-dlp's download, its progress lines, the cover art
        library/    the library's file names and the names Windows can't use
        tagging/    the Tagger
    platform/       service interfaces
    platform/win32/ Win32 implementations of the platform services
    ui/             Win32 windows, EventSink implementation
    main.cpp        wWinMain: wires platform services, core and UI together
tests/
    ui/             the UI checks, with a fake yt-dlp and a local stand-in for the web services
    unit/           unit checks of the core's pure logic, so far the Matcher's rules against real cases
config/
    remote.json     served to installations via raw.githubusercontent.com
```
