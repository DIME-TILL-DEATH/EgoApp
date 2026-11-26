# EgoApp Project Context

EgoApp is AMT's desktop application for AMT EgoGig4 users. It manages playlists and prepares media on an SD card or local folder for later transfer.

## Constraints
- User-data preservation is critical.
- The numbered .ego file format is a compatibility contract and must never be changed as part of playlist refactoring or nested-folder support.
- macOS: Qt 6.7, minimum macOS 11.
- Windows: Qt 6.11/6.12, minimum Windows 10 1809.
- Linux: Qt 6.11/6.12; minimum Ubuntu 22.04.
- Primary audio conversion uses Qt.
- SoX converter is legacy; do not modify third-party SoX library internals.

## Source map
Verify paths against the current tree.
| Area | Files |
|---|---|
| Playlist feature | MainApplication/playlists/playlistservice.cpp/.h, playlistmodel.cpp/.h, playlistrepository.cpp/.h, playliststorage.cpp/.h, song.cpp/.h |
| SdContentModel | MainApplication/sdcontentmodel.cpp/.h |
| UiCore | MainApplication/uicore.cpp/.h |
| Player | MainApplication/soundplayer.cpp/.h |
| Decoder | MainApplication/decodestream.cpp/.h |
| QML UI | MainApplication/qml/*.qml, compiled as the EgoApp module |
| Entry point | MainApplication/main.cpp |
| Legacy resampler | WavConverterLib/resampler.cpp/.h |
| Legacy shell | WavConverterShell/CMakeLists.txt |

## Data paths
**Playlists:** QML -> UiCore -> PlaylistService -> PlaylistModel / PlaylistRepository. PlaylistService owns the active workspace path, coordinates playlist lifecycle and model construction. UiCore passes the selected workspace to PlaylistService and SdContentModel without retaining a duplicate QDir member. It validates one playlist by its slash-separated relative path, using song records loaded from PlaylistRepository/PlaylistStorage and the service-owned workspace root; validation must not query PlaylistModel roles or indexes. PlaylistRepository recursively discovers playlist directories and safely creates/removes slash-separated paths relative to PLAYLIST/. PlaylistModel owns per-playlist song state and Qt Model/View behavior; PlaylistStorage reads and writes numbered .ego files. Loading validates numeric filenames, duplicate/out-of-range numbers, UTF-8 record contents and file-read errors; invalid records are skipped and reported rather than silently converted into empty songs. PlaylistRepository carries per-playlist load diagnostics to PlaylistService. UiCore's constructor only establishes internal signal connections. The QML root calls UiCore.initialize() from Component.onCompleted; this initializes the workspace, loads playlists, and configures the language after QML signal connections are ready. Once initialization is complete, UiCore emits workspaceChanged so QML bindings to workspacePath and avaliablePlaylists refresh with the loaded values. Playlist load diagnostics are delivered directly through playlistLoadIssuesFound, without a startup buffer. Playlists with read errors are made read-only to prevent a later save from deleting records that could not be loaded. Keep Qt Model/View concerns out of storage and repository code, and keep filesystem lifecycle logic and playlist validation out of UiCore. A playlist with .ego loading, filename, or numbering errors exposes its `corrupted` state to QML: selecting it shows a dedicated warning, editing and playback controls are disabled, and the selector entry is dimmed. Missing WAV/media files are validation issues only; they do not mark the playlist corrupted or disable it.

**Nested playlist directory rules:**
- A playlist path is a slash-separated relative path under PLAYLIST/, e.g. Collections/Rock. It is the stable playlist identifier; the final path component is the display name.
- Every directory below PLAYLIST/ is exposed as a playlist. A directory may itself contain .ego files and child playlist directories.
- Existing top-level playlists remain valid without migration. Their identifiers remain their directory names.
- Absolute paths, empty path components, dot/dot-dot components, hidden components, drive-qualified paths, backslashes, control characters, and symbolic-link traversal are rejected.
- PlaylistModel receives the workspace path explicitly; it must not infer it by walking upward from its playlist directory because playlists may be nested.
- The playlist UI is planned as a folder tree: users select a folder, expand it to inspect nested playlists, and create a child playlist by entering only its name. Do not require users to type a slash-separated path in the creation dialog. Removing a directory removes its entire nested subtree.

**Qt Model/View contract:**
- PlaylistModel is a flat list and returns zero rows for a valid parent index.
- moveSong() uses final row indexes; its implementation translates them to Qt moveRows() destination-child insertion positions. Moving to the end is supported.
- dataChanged() notifications specify the roles affected by each song update.
- insertRows() accepts only positions from zero through rowCount() inclusive and rejects counts that would overflow the model's int row range; removeRows() validates count against the remaining rows without adding position and count.
- moveSong() reports success. PlaylistEditor.qml restores the DelegateModel visual ordering when the source-model move fails, and only adjusts the selected row after a successful removal.

**Audio import:** Qt is the primary conversion path. On failure, source data and previous destination assignment must remain available.
**Playback:** check decoder/stream ownership, track switching, stop, errors, teardown, and buffering of long files.

## Previous static-audit areas
These are areas to re-check, not confirmed defects in current HEAD:
- Failure-safe multi-file .ego updates and rollback.
- Preserving previous destination on failed import.
- Import queue terminal state after errors.
- Player/decoder lifetime and memory growth.
- Remaining UiCore/SdContentModel responsibility split (workspace policy, validation, localization, media import).
- Automated tests for playlist persistence and Qt Model/View contracts are deferred until playlist architecture work is complete.


**Media paths stored in .ego (critical path invariant):**
- The workspace is the root of the target SD-card content. Media paths in .ego records are rooted at this workspace, not at the playlist directory and not at the host operating system's filesystem root.
- The existing UI writes paths such as `/SONGS/Artist/Track.wav`. The leading slash is a workspace-root marker: resolve it as `<workspace>/SONGS/Artist/Track.wav`.
- Never change this behavior by passing the stored leading-slash path directly to host filesystem absolute-path resolution, resolving it relative to the playlist directory, or anchoring it to the application/current working directory.
- Validate path components before joining to the workspace. Reject traversal components (`.`, `..`), malformed/empty components, drive-qualified paths, control characters, and unsupported separators rather than silently cleaning them into a different path. Do not reject the leading slash solely because it is present.
- For backward compatibility, a path without the leading slash may be interpreted from the workspace root too; it must never be interpreted from the playlist directory.
- The app currently generates forward-slash paths. Symbolic links on the target device are not a supported media-path scenario; do not add a dedicated symlink feature without an explicit requirement.
- Keep this distinct from playlist identifiers: playlist identifiers are relative to `PLAYLIST/`; media paths are rooted at the workspace.


**Shared media path resolution:** PlaylistService and PlaylistModel use the common `MediaPath::resolveWorkspacePath()` helper so both layers interpret stored media paths consistently. Validation policy is kept separate from path resolution.


**Workspace-root marker implementation:** the shared resolver removes exactly one leading forward slash from an .ego media path and joins the remainder to the workspace directory. It does not pass the stored leading-slash path to host absolute-path resolution. Paths without the marker are also interpreted from the workspace root for compatibility.


**Media path component validation:** the common resolver rejects empty paths, repeated/trailing separators, backslashes, drive/colon syntax, control characters, and `.`/`..` components before joining to the workspace. It deliberately accepts one leading slash as the workspace-root marker. This combines lexical validation with a simple rejection of existing symbolic-link components; symlink targets are never followed as supported media paths.


**.ego read strategy:** each numbered record is read in one `QFile::readAll()` operation. The I/O error is checked immediately before classifying empty/incomplete content or decoding UTF-8. The byte buffer is then parsed using the existing two-line format; an absent/empty second line remains valid and extra data after the second line remains an error.


**.ego filesystem race handling:** directory enumeration uses `QDir::NoSymLinks` and checks that the playlist directory remains present/readable after listing. Each listed file is rechecked before opening; open failures are reported with the file error. Size and modification time are compared before and after reading to detect common concurrent replacement/modification cases. Qt's `QDir::entryList()` does not expose a reliable enumeration error status, so an empty result from a directory that remains readable is treated as an empty playlist; detecting partial native directory-enumeration failures would require platform-specific APIs. Metadata comparison is best-effort and cannot detect every same-size/same-timestamp rewrite.


**Playlist media validation:** `checkPlaylist()` resolves stored media paths through the shared workspace resolver, verifies that resolved targets exist and are regular files, and applies the device's Latin-character filename rule to the stored .ego path rather than the host mount path. Diagnostics identify the actual .ego file number (`Song::fileNum`), not the current row. PlaylistModel missing-file roles use the same resolver and regular-file check. MIDI-existence checks are limited to an existing WAV track and a regular sibling .mid file. No additional extension restriction is imposed by checkPlaylist until the device's accepted media formats are explicitly confirmed.


Media path resolution also refuses a path if any existing component below the workspace is a symbolic link. Such paths are treated as unavailable; the application does not follow or otherwise process media symlinks on the target device.
