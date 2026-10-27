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
|   StateStore      Settings      ToolManager      SelfUpdater             |
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
    logs\<timestamp>.log                  one log per run, including full tool output

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
  "delayBetweenTracksSeconds": 3,
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
| `delayBetweenTracksSeconds` | pause between track starts                                                    |
| `autoUpdateTools`           | update installed tools at startup (see ToolManager)                           |

So far the app reads and writes `version`, `libraryPath` and `autoUpdateTools`; the other fields arrive with their
features. The file
is pretty-printed UTF-8, indented by 4 spaces. Keys it doesn't know are skipped and missing ones keep their
defaults, so files from other versions still load. Without a file the app runs on the defaults and writes nothing
until a setting changes.

### state.json

Pretty-printed UTF-8, read and written with glaze. Fields appear in struct declaration order. `playlists`,
`sources` and `tracks` are `std::map`s, so their keys are sorted. File paths are relative to `libraryPath`.

The state separates what playlists list from what is on disk:

- A **source** is one entry in an online catalog: a YouTube Music video or a Spotify track. Its key is
  `<source>:<source id>`, e.g. `ytm:<videoId>` or `spotify:<trackId>`. Playlists list source keys, and a source
  that appears in several playlists has one entry.
- A **track** is one audio file in the library, made from one YouTube video. It has one or more sources. The
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
      "isrc": "USxxx0000000",
      "coverUrl": "https://i.scdn.co/image/...",
      "match": { "videoId": "ccccccccccc", "confidence": 0.94, "review": false,
                 "title": "Title", "artists": [ "Artist One" ], "durationMs": 243000 },
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
      "videoId": "aaaaaaaaaaa",
      "edits": { "album": "Album (Deluxe)" },
      "status": "downloaded",
      "file": "Artist - Title.opus",
      "addedAt": "2026-10-11T14:03:00Z"
    },
    "spotify:6r8k1vznHrzlEKYxxxxxxx": {
      "sources": [ "spotify:6r8k1vznHrzlEKYxxxxxxx", "ytm:ccccccccccc" ],
      "videoId": "ccccccccccc",
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
  }
}
```

In this example, the WCS playlist (YouTube Music) and Her playlist (Spotify) both contain "Title". The Matcher
picked the same video (`ccccccccccc`) that WCS lists, so the two sources share one track and one file.

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
| `album`, `year`                  | Spotify: the album from the playlist response; its answer has no year. YouTube Music: from the info JSON (`album`, `release_year`), once the source's video has been downloaded |
| `isrc`, `coverUrl`               | Spotify only. The playlist response has the cover but no ISRC; where the ISRC comes from is decided with the Matcher |
| `match`                          | Spotify only: the Matcher's result, with `videoId`, `confidence` (0 to 1), `review`, and the video's `title`, `artists` and `durationMs` as YouTube Music's search lists them, so a review shows what was matched |
| `track`                          | Key of the track this source belongs to                                    |

**Track fields.**

| Field              | Meaning                                                                         |
|--------------------|---------------------------------------------------------------------------------|
| `sources`          | Its source keys, in the order they joined. The first one created the track; its key is the **track key**, which never changes |
| `videoId`          | The YouTube video the file was made from                                        |
| `videoOverride`    | See Video resolution                                                            |
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

**Video resolution.** A track's video is the first of:
1. its `videoOverride`;
2. the video of its first YouTube Music source, because the playlist owner chose it;
3. the Matcher's result for its first Spotify source.

`resolved_video` (`src/core/state/TrackVideos`) gives the video and the source it comes from; there is no override yet.

A track **needs review** when its video comes from a match with `review: true`. Resolution runs for tracks
without a file, and again when the override changes. Setting `videoOverride` re-downloads the track (or merges
it, see below), and clears `failure` and `review`.

**Merging.** No two tracks share a video. A video gets resolved in three ways:
- a fetch adds a YouTube Music source;
- the Matcher finishes;
- a video override is set.

Each time, if another track already has that video, the two tracks merge:
- The track that had the video first survives. The other track's sources move into it, and its entry is
  deleted.
- If the deleted track had a file, that file goes to the Recycle Bin.
- The surviving track keeps its own `edits`, and takes the other track's edits for fields it hasn't edited.
- When the merge changes the effective metadata or cover, the file is re-tagged and renamed.
- The engine emits `TracksMerged`.

Sources stay attached to their track until the track is removed.

**Tracks that leave every playlist.** After a fetch and after `removePlaylist`, a track none of whose sources is in a
playlist anymore is looked at again (`prune_tracks`, `src/core/state/`):
- A track without a file (`pending`, `failed`) is deleted with its sources: there is nothing in the library to decide
  on.
- A track with a file becomes `orphaned` (see SyncEngine › Orphans).

**Effective metadata.** Tags and file names use, per field, the first of:
1. `edits`;
2. the track's first Spotify source, because Spotify metadata is the cleanest (separate artists, proper album,
   ISRC);
3. the track's first YouTube Music source.

The cover art comes from the first of:
1. the custom cover;
2. the first Spotify source's `coverUrl`;
3. the video thumbnail.

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
- `ToolsStore` (`src/core/tools/`) reads and writes it through `FileOps`, in the same way as `SettingsStore`: pretty-printed,
  unknown keys skipped. A file that doesn't read counts as no file, because the tools can always be downloaded again.

## Platform services

| Service          | Implementation                                                                       |
|------------------|--------------------------------------------------------------------------------------|
| `HttpClient`     | WinHTTP (`WinHttpClient`). `get` sends the request headers it is given and returns a body; `download` streams into a file and reports the bytes received as they arrive. Automatic proxy, redirects followed, timeouts, and a `User-Agent: WestieLib/<version>` that GitHub's API requires. A non-2xx answer throws `HttpError`, which keeps the status and reads `HTTP <code> from <host>`, a connection problem `Couldn't reach <host>: <WinHTTP's message>`. A `std::stop_token` cancels: its callback closes the request handle, which ends a blocked call, and the call throws `Cancelled`. |
| `ProcessRunner`  | `Win32ProcessRunner`. `run(program, arguments, stop)` starts the program with `CreateProcessW`, without a console window, and returns its exit code, stdout and stderr. Arguments and output are UTF-8; the arguments are quoted by the C runtime's rules. A child inherits only its own pipe ends and NUL as stdin (`PROC_THREAD_ATTRIBUTE_HANDLE_LIST`), so children started at the same time never hold each other's pipes open. It starts suspended, joins a Job Object with `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`, and only then runs. A `std::stop_token` cancels: its callback terminates the job, and the call throws `Cancelled`. Children get `PYTHONUTF8=1` and `PYTHONIOENCODING=utf-8`. So far it returns the whole output at the end; splitting it into lines as they arrive comes with the Downloader's progress. |
| `FileOps`        | Atomic replace: a flushed `.tmp` renamed over the file, or through `ReplaceFileW` when a backup is kept (`replace_file_keeping_backup`), moves via `MoveFileExW` with `MOVEFILE_COPY_ALLOWED`, Recycle Bin deletes via `IFileOperation` with `FOFX_RECYCLEONDELETE`. |
| `Hasher`         | SHA-256 of a file via BCrypt (`BCryptHasher`), as lowercase hex.                     |
| `ArchiveExtractor` | miniz (`MinizArchiveExtractor`). Extracts the named files from a zip into a folder. miniz opens files by narrow name, so it gets a `FILE*` from `_wfopen`, and writes go through `CreateFileW`. A stop request ends it with `Cancelled`. |
| `TextNormalizer` | `NormalizeString` (NFKD), combining marks dropped, case folded.                      |

Strings inside the core are UTF-8 `std::string`; paths are `std::filesystem::path`. Conversion to UTF-16 happens
inside the platform services.

## Core components

### Settings and StateStore

`SettingsStore` (`src/core/settings/`) loads and saves `settings.json` as a `Settings`, through `FileOps`, and holds the
defaults that `main.cpp` gives it. `main.cpp` loads it before any window shows. A file that doesn't read stops the
app with a message that names the file, rather than writing the defaults over it. Each change is saved at once:
`FileOps::replace_file` writes `settings.json.tmp`, flushes it, and renames it over `settings.json`.

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
  `fetchErrorAt`; sources with `title`, `artists`, `album` (left out when empty), `durationMs`, `match` (without
  `review`) and `track`; and tracks with `sources`, `status`, `failure`, `error` and `addedAt`. Empty fields are left
  out, and a failure name from a later version reads as none. The other fields arrive with their features.

A named mutex makes WestieLib single-instance, so exactly one process writes the state.

### RemoteConfig

`RemoteConfig` fetches `https://raw.githubusercontent.com/Rheel17/WestieLib/main/config/remote.json` at startup and
caches it as `remote.json`. A compiled-in default is used until the first successful fetch. It carries the Spotify GraphQL operation name and persisted-query hash:

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

So far the app downloads yt-dlp and ffmpeg; deno follows.

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
(`lock_for_use`), and `ToolInstaller` holds the exclusive one while it moves the new files in (`lock_for_replace`). So an
update waits for running tools to end, and a tool never starts halfway through an update. `main.cpp` makes it, and hands
it to ToolManager and to the components that run tools.

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
| `searchVideos`                         | yt-dlp         |
| Downloader (audio, thumbnail crop)     | yt-dlp, ffmpeg |

A step whose tool isn't available stays queued, and the track stays `pending` with a `waitingFor` list of tool names
in its `TrackStatusChanged` event; it isn't a failure, and nothing is written to the state for it. When ToolManager
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
	std::string key;                       // source key: "ytm:<videoId>" / "spotify:<trackId>"
	std::string title;
	std::vector<std::string> artists;
	std::string album;
	std::optional<std::string> isrc;
	std::optional<std::string> coverUrl;
	std::optional<std::string> videoId;    // set by YouTubeMusicSource
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
`videoId`. The album and year of these sources come from yt-dlp's info JSON at download time.

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
- So far a source keeps the title, artists, album and duration; the cover URL arrives with the Downloader.
- Errors:
  - the embed page's error page, or `playlistV2` of type `NotFound`: "The playlist doesn't exist or is private.";
  - an answer with `errors` (`PersistedQueryNotFound` when the hash is stale): "Spotify said: <message>";
  - any HTTP error as `HttpClient` words it. Spotify's servers answer a page with HTTP 500 now and then, and the same
    request goes through a moment later, so a page that gets a 5xx is asked for once more, a second later; a stop
    request ends that wait at once.

### Matcher

The Matcher turns a Spotify source into a YouTube Music `videoId`. It runs only for tracks that have no video
override and no YouTube Music source (see Video resolution).

1. **Queries.** The ISRC first (when present), then `"<artists> <title>"`.
2. **Search.** `yt-dlp --flat-playlist -J --playlist-items 1:10 "https://music.youtube.com/search?q=<query>#songs"`.
   Each candidate carries videoId, title, artist and duration.
3. **Normalize** titles and artists with `TextNormalizer`, strip punctuation and bracketed suffixes such as
   `(feat. ...)` and `- Remastered 2011`.
4. **Score** each candidate as a weighted sum of title similarity, artist similarity and duration closeness. Words
   such as `live`, `remix`, `cover`, `karaoke`, `sped up`, `slowed`, `instrumental` lower the score unless the
   Spotify title contains the same word. A candidate whose video already belongs to a track gets a small bonus,
   so the same song from both sources tends to end up as one track.
5. **Choose** the highest score.
   - The source's `match` stores the video id, the confidence, and the candidate's title, artist and duration.
   - A confidence below the review threshold sets `match.review = true`. The track is downloaded with its best
     candidate, and it stays flagged until a video override is set or the match is confirmed.
   - If the chosen video already belongs to another track, the two tracks merge instead of downloading.
   - When the searches return nothing, or no candidate reaches the minimum score, the track fails with
     `failure: noMatch` and the searched queries in `error`.

The Matcher is pure logic on top of `ProcessRunner` output and is unit-tested with recorded yt-dlp JSON.

**Video search.** `searchVideos(query, filter)` serves the UI's Choose a video dialog. It runs the same yt-dlp
search with `#songs`, `#videos`, or no fragment (for both), and returns up to 20 results. Each result has
videoId, title, artist, album, type (song or video) and duration. A search runs on its own worker thread, outside
any sync run. A new search cancels the previous one.

### Downloader

The Downloader produces one tagged `.opus` or `.flac` file in `work\<track key>\`:

```
yt-dlp -f "bestaudio[acodec=opus]/bestaudio" -x --audio-format <format>
       --write-info-json --write-thumbnail --convert-thumbnails jpg
       --ffmpeg-location <tools> --js-runtimes deno:<tools>\deno.exe
       --newline --progress-template "download:%(progress._percent_str)s"
       --no-playlist -o "<work>\%(id)s.%(ext)s"
       https://music.youtube.com/watch?v=<videoId>
```

- **Format.** `<format>` comes from `settings.format` at download time.
  - `opus` (the default): Opus sources are remuxed into `.opus` without re-encoding, and AAC sources are
    transcoded to Opus.
  - `flac`: the audio is transcoded to FLAC. The source is lossy, so FLAC changes the container but not the
    quality.

  A format change applies to new downloads only. Existing files keep their format, so a library can mix both.
- Progress lines from `--progress-template` become `TrackProgress` events.
- **Cover art.** The first of these that applies:
  1. a custom cover from `edits` (`covers\<track key>.jpg`);
  2. when the track has a Spotify source, its Spotify image (square, 640 px), fetched with `HttpClient`;
  3. the video thumbnail, center-cropped to a square with `ffmpeg -vf "crop=min(iw\,ih):min(iw\,ih)"`.

  No cover is fetched when `embedCover` is off.
- **Metadata.** The info JSON fills the album and year of the track's YouTube Music sources: `track` (falling
  back to `title`), `artists` (falling back to `artist`), `album`, `release_year`. The tags then use the
  effective metadata.

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
| `WESTIELIB_VIDEOID`      | YouTube video id the audio came from                          |

**Cover art** is the front cover (picture type 3), as a JPEG. It is written only when `embedCover` is on:
- in Opus files, as `METADATA_BLOCK_PICTURE`;
- in FLAC files, as a native `PICTURE` block.

`WESTIELIB_SOURCE` and `WESTIELIB_VIDEOID` make every file self-identifying, which powers state rebuilds.

The Tagger also re-tags finished library files in place, after `editTrack` and after a merge.

### Library

The Library places finished files and handles the library folder as a whole.

- **File names.** `<ARTIST> - <TITLE>.<opus|flac>`, built from the effective metadata. Characters `<>:"/\|?*` and control characters become `_`, trailing dots
  and spaces are trimmed, reserved device names (`CON`, `NUL`, `COM1`, ...) get a `_` suffix, and names are capped at
  200 characters. A name that already belongs to another track gets ` (2)`, ` (3)`, ... appended.
- **Placement.** The tagged file moves from `work\` into the library with `MoveFileExW`, so the library only ever
  contains complete files.
- **Rename.** When `editTrack` or a merge changes the effective artists or title, the file is renamed with the
  same rules.
- **Removal.** Files go to the Recycle Bin through `FileOps`.
- **Relocation.** `relocateLibrary(newPath)` updates `libraryPath`; all `file` entries are relative, so they stay valid.
- **Rebuild.** `rebuildState()` scans the library folder for `.opus` and `.flac` files. From each one it reads
  `WESTIELIB_SOURCE` / `WESTIELIB_VIDEOID` and the regular tags, and recreates the track and its sources.
  - The file tags become the source values, so `edits` are not restored.
  - Playlist membership is restored on the next sync.

### SyncEngine

The SyncEngine coordinates a sync run on a coordinator thread with a pool of `concurrency` worker threads.

1. **Fetch.** For each selected playlist, the matching `PlaylistSource` returns the ordered source list.
   - On success, the playlist's `sources` and `name` in the state are replaced with it, and `fetchError` is
     cleared. The engine emits `PlaylistFetched`.
   - A new source becomes a new `pending` track with `addedAt`. The exception is a new YouTube Music source
     whose video already belongs to a track: it joins that track (see Merging).
   - On failure, the playlist gets `fetchError` / `fetchErrorAt` and keeps its previous source list, plus the pages
     a Spotify fetch read before it failed, so none of its tracks become orphans. The engine emits `PlaylistFetchFailed`, and the run continues with the other
     playlists.
2. **Orphans.** `downloaded` tracks with no source in any playlist become `orphaned`. The engine emits one
   `RemovalsPending` event with all of them.
3. **Queue.** All `pending` and `failed` tracks are queued in playlist order. Workers start tracks
   `delayBetweenTracksSeconds` apart.
4. **Per track (worker).**
   1. Resolve the video (see Video resolution). This step takes a lock on the video id, so two tracks that
      resolve to the same video in one run merge instead of downloading it twice.
   2. If the video already belongs to another track, merge, emit `TracksMerged`, and stop here.
   3. Downloader, then Tagger, then Library placement.
   4. Update the track (`downloaded`, `file`, `videoId`) and the source's `match`, or mark the track `failed`
      with a `failure`:
      - `noMatch`, with the searched queries as `error`;
      - `download`, with the last lines of tool output as `error`.
   5. Save the state, clean `work\<track key>\`, emit `TrackStatusChanged`.
5. **Finish.** Emit `SyncFinished` with counts per status.

**Retry.** `retry(trackKeys)` sets the tracks to `pending` and queues them without a fetch step. While a run is
active, they join its queue. Otherwise they start a run of their own.

**Removal decisions** arrive through `resolveRemovals` at any time during or after a run.
- `remove` sends the file to the Recycle Bin and deletes the track and its sources.
- `keep` sets the status to `kept`.

A `kept` or `orphaned` track that one of its sources brings back into a playlist returns to `downloaded`.

**Cancellation** sets the run's `CancelToken` and terminates the Job Object. In-flight tracks return to `pending`
and their work folders are cleaned.

**So far** (`src/core/sync/`), a run only fetches, YouTube Music and Spotify playlists alike.
- `sync(keys)` queues the playlists that aren't queued yet and emits `SyncQueued`. Without a run it starts one on a
  worker thread (`std::jthread`); during a run they join its queue. The queue is behind a mutex, so a run that finds
  it empty and ends can't miss a `sync` that comes at the same moment.
- Per playlist the run emits `PlaylistFetching` and fetches. `apply_fetch` (`ApplyFetch.cpp`) puts the result in
  the state: the playlist's `name` and `sources`, `lastSync`, no `fetchError`, the source values from the playlist,
  a `pending` track for each new source (keyed like the source), then `prune_tracks`. A playlist removed during
  the fetch is left alone. Then the run emits `PlaylistFetched`.
- A Spotify fetch also hands over the tracks read so far after every page but the last. `apply_fetch_page` puts them
  in the state: the playlist's `name`, and as its `sources` the ones read so far followed by its earlier sources that
  haven't come back yet, with the source values and new `pending` tracks as above. It leaves `lastSync`, `fetchError`
  and the pruning to the end, so no track leaves a playlist halfway. Then the run emits `PlaylistPageFetched`.
- A failed fetch stores `fetchError` / `fetchErrorAt` and emits `PlaylistFetchFailed`; the run goes on. The pages a
  Spotify fetch read before it failed stay, beside the playlist's earlier sources.
- Without yt-dlp, a YouTube Music fetch emits `PlaylistFetching` with `waitingFor`, and waits until ToolManager installs it
  (`ToolFolder::wait_until_installed`, which `ToolInstaller` signals after moving the files in).
- `cancel()` and `stop()` end the run. The playlist being fetched keeps its previous state, apart from the pages a
  Spotify fetch already read (as for a failure); the queue is dropped, and nothing more is written.
- Every run ends with `SyncFinished`, cancelled or not.

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
| `setVideoOverride(trackKey, youtubeUrl)` | Sets `videoOverride` and re-downloads the track, or merges it into the track that already has that video |
| `confirmMatch(trackKey)`                 | Clears `review` on the match the track's video came from       |
| `searchVideos(query, filter)`            | Searches YouTube Music songs, videos or both; answers with `VideoSearchResults` |
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
| `TrackStatusChanged`           | track key, status, failure, error, the tools it waits for |
| `TrackProgress`                | track key, percent                              |
| `TrackUpdated`                 | track key (tags, cover or file changed)         |
| `TracksMerged`                 | deleted track key, surviving track key          |
| `VideoSearchResults`           | query, filter, results                          |
| `RemovalsPending`              | list of orphaned track keys                     |
| `ToolsRunChanged`              | whether a ToolManager run is going              |
| `ToolStatus`                   | tool, state, installed release, percent, error  |
| `ToolsChecked`                 | when the releases were last looked up           |
| `AppUpdate`                    | available version, update state                 |
| `LogLine`                      | level, text, optional playlist id and track key |

Events are emitted from any thread through an `EventSink` interface, as a `CoreEvent` (a `std::variant` of the event
structs). The Win32 implementation, `CoreEventQueue` (`src/ui/events/`), appends each event to a queue behind a
mutex. When the queue was empty, it posts one message without a payload to the main window, which then takes every
queued event at once. The queue owns the events, so nothing leaks when the window is gone before it takes them.

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
        playlists/  playlist links, the playlist look-up, addPlaylist
        settings/   Settings, SettingsStore
        sources/    SourceType, each source's playlist name, the Spotify embed page, the YouTube Music and Spotify fetches
        state/      State, StateStore, prune_tracks
        sync/       SyncEngine, apply_fetch and the sync events
        tools/      ToolManager, ToolInstaller, ToolSource, ToolsStore, ToolFolder, YtDlp and the tool types
                    later areas get folders of their own: Matcher, Downloader, Tagger,
                    Library, SelfUpdater, RemoteConfig
    platform/       service interfaces
    platform/win32/ Win32 implementations of the platform services
    ui/             Win32 windows, EventSink implementation
    main.cpp        wWinMain: wires platform services, core and UI together
tests/              core unit tests with fake platform services and recorded tool output
config/
    remote.json     served to installations via raw.githubusercontent.com
```
