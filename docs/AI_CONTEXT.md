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
| Audio subsystem | MainApplication/audio/soundcore.cpp/.h, soundplayer.cpp/.h, decodestream.cpp/.h, audiomixer.cpp/.h |
| QML UI | MainApplication/qml/*.qml, compiled as the EgoApp module |
| Entry point | MainApplication/main.cpp |
| Legacy resampler | WavConverterLib/resampler.cpp/.h |
| Legacy shell | WavConverterShell/CMakeLists.txt |

## Data paths
**Playlists:** QML -> UiCore -> PlaylistService -> PlaylistModel / PlaylistRepository. PlaylistService owns the active workspace path, coordinates playlist lifecycle and model construction. UiCore passes the selected workspace to PlaylistService and SdContentModel without retaining a duplicate QDir member. It validates one playlist by its slash-separated relative path, using song records loaded from PlaylistRepository/PlaylistStorage and the service-owned workspace root; validation must not query PlaylistModel roles or indexes. PlaylistRepository recursively discovers playlist directories and safely creates/removes slash-separated paths relative to PLAYLIST/. PlaylistModel owns per-playlist song state and Qt Model/View behavior; PlaylistStorage reads and writes numbered .ego files. Loading validates numeric filenames, duplicate/out-of-range numbers, UTF-8 record contents and file-read errors; invalid records are skipped and reported rather than silently converted into empty songs. PlaylistRepository carries per-playlist load diagnostics to PlaylistService. UiCore's constructor only establishes internal signal connections. The QML root calls UiCore.initialize() from Component.onCompleted; this initializes the workspace, loads playlists, and configures the language after QML signal connections are ready. Once initialization is complete, UiCore emits workspaceChanged so QML bindings to workspacePath and avaliablePlaylists refresh with the loaded values. Playlist load diagnostics are delivered directly through playlistLoadIssuesFound, without a startup buffer. Playlists with read errors are made read-only to prevent a later save from deleting records that could not be loaded. Keep Qt Model/View concerns out of storage and repository code, and keep filesystem lifecycle logic and playlist validation out of UiCore. A playlist with .ego loading, filename, or numbering errors exposes its `corrupted` state to QML: selecting it shows a dedicated warning, editing and playback controls are disabled, and the selector entry is dimmed. Missing WAV/media files are validation issues only; they do not mark the playlist corrupted or disable it. Playlist-check/loading diagnostics wrap within the results window, and long combined track names in the playlist editor are middle-elided to stay within the row width.

**Nested playlist directory rules:**
- A playlist path is a slash-separated relative path under PLAYLIST/, e.g. Collections/Rock. It is the stable playlist identifier; the final path component is the display name.
- Every directory below PLAYLIST/ is exposed as a playlist. A directory may itself contain .ego files and child playlist directories.
- Existing top-level playlists remain valid without migration. Their identifiers remain their directory names.
- Absolute paths, empty path components, dot/dot-dot components, hidden components, drive-qualified paths, backslashes, control characters, and symbolic-link traversal are rejected.
- PlaylistModel receives the workspace path explicitly; it must not infer it by walking upward from its playlist directory because playlists may be nested. Every song mutation entry point rejects corrupted playlists before preparing changes; the persistence layer retains a final guard. `setLink()` and `setPlayNext()` report success, and row indexes must belong to the receiving model.
- The playlist UI is planned as a folder tree: users select a folder, expand it to inspect nested playlists, and create a child playlist by entering only its name. Do not require users to type a slash-separated path in the creation dialog. Removing a directory removes its entire nested subtree.

**Qt Model/View contract:**
- PlaylistModel is a flat list and returns zero rows for a valid parent index.
- moveSong() uses final row indexes; its implementation translates them to Qt moveRows() destination-child insertion positions. Moving to the end is supported.
- dataChanged() notifications specify the roles affected by each song update.
- insertRows() accepts only positions from zero through rowCount() inclusive and rejects counts that would overflow the model's int row range; removeRows() validates count against the remaining rows without adding position and count.
- moveSong() reports success. PlaylistEditor.qml restores the DelegateModel visual ordering when the source-model move fails, and adjusts the selected row only after successful insertion/removal.

**Audio import:** Qt is the primary conversion path. On failure, source data and previous destination assignment must remain available.
**Playback:** `SoundCore` validates workspace-backed media paths, selects the requested source, and exposes playback state to QML. It resets the QML-facing position and duration before calling `SoundPlayer::play()`, because decoder notifications can arrive during the start call and must not be overwritten afterward. It also publishes the requested playback metadata before starting asynchronous decoding, so an immediate decoder error cannot race ahead of metadata assignment and leave a stale song/source label. Its `PlayPause(mode, index)` method starts a new source when the selected item differs from the active one and toggles pause/resume when it matches. The generic `playbackActive` and `playbackPaused` properties expose the SoundPlayer lifecycle; `egoPlaybackActive` and `sdPlaybackActive` identify which source panel owns the active playback so the inactive panel always shows Play. `SoundPlayer` coordinates decoder streams and an injectable `AudioOutput` interface; the production `QAudioSinkOutput` adapter owns the physical `QAudioSink`, while tests inject a fake output without requiring an audio device; `DecodeStream` decodes one file and spools PCM to a temporary file so seeking does not require retaining the complete decoded audio in RAM. The stream is opened unbuffered so QIODevice prefetching cannot desynchronize the spool cursor from byte-availability accounting; seeking beyond the currently decoded extent is supported while later decoder buffers append at the end without moving the read cursor. It derives a progressively increasing duration from the decoded PCM byte count and output format, while also accepting a larger duration reported by the decoder; this avoids relying on optional WAV duration metadata. Access to the spool and mixer state is serialized because decoder callbacks and the QAudioSink pull path may run on different threads. `AudioMixer` is a hardware-independent `QIODevice` that mixes two PCM streams, applies per-track channel/volume/mute settings, and reports consumed playback position. The position signal chain is `AudioMixer::positionUpdated` → `SoundPlayer::positionUpdated` → `SoundCore::trackPositionChanged` → `qmlTrackPositionChanged`; QML binds `PlayerPanel.value` to the corresponding SoundCore position property. Each DecodeStream decoder instance belongs to one playback session; closing invalidates that session before stopping and retiring its decoder, so delayed callbacks from an old source cannot mutate a new session. Decoder errors are terminal failures, distinct from successful decoding completion. PCM spooling is capped at one hour per track (44.1 kHz, stereo, signed 16-bit; about 606 MiB per track); exceeding the cap is reported as a decoding/storage error. Decoder data arrival and successful completion are explicit signals; temporary buffer starvation is not end-of-track. While both tracks are still decoding, mixing waits for data from both to preserve synchronization; a completed shorter track contributes silence while the longer track continues. A `QModelIndex` supplied by `SdProxyModel` is transient: callers must obtain a fresh index for each action and must not retain it across proxy/source model updates or resets. `SoundCore::PlayPause()` validates the proxy source model before mapping the index. The `audio_tests` target exercises the mixer, decoder lifecycle, and `SoundPlayer` state transitions through a fake `AudioOutput`, without requiring a physical audio device. Re-check decoder/stream ownership, track switching, stop, pause/resume, natural drain, output/decode errors and their propagation to SoundCore, metadata cleanup after terminal errors or natural completion, teardown, seek during decoding, and long-file buffering.

## Previous static-audit areas
These are areas to re-check, not confirmed defects in current HEAD:
- Failure-safe multi-file .ego updates and rollback.
- Preserving previous destination on failed import.
- Import queue terminal state after errors.
- Player/decoder lifetime and memory growth.
- Remaining UiCore/SdContentModel responsibility split (workspace policy, validation, localization, media import).
- Automated playlist tests for storage, repository, service, and Qt Model/View behavior are implemented with Qt Test in `MainApplication/tests/playlist_tests.cpp`; see the test instructions below.


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

**Playlist playback path contract:** SoundCore validates the supplied PlaylistModel/index, rejects structurally corrupted playlists, and resolves every non-empty stored track path through the model's workspace-rooted MediaPath resolver. A song may contain only track 1, only track 2, or both; at least one linked track is required. Each linked track must resolve to an existing regular file. SoundPlayer reports both synchronous stream-open failures and asynchronous decoder errors. Playback duration is the longer decoded track duration, and the shorter or absent stream contributes silence after it ends. Playback never derives the workspace root from the playlist directory depth.

**Playlist root availability:** a missing PLAYLIST/ directory is treated as a valid empty workspace. An unavailable/unreadable workspace or an existing PLAYLIST/ path that is not a supported readable directory is reported through playlist load diagnostics. If the root becomes unavailable during tree loading, the partial snapshot is discarded. Platform-specific diagnosis of partial directory enumeration remains out of scope.


**Playlist persistence strategy:** PlaylistModel supplies the previous and proposed song lists plus the affected row interval to PlaylistStorage. Storage compares serialized records and touches only changed numbered `.ego` files. Single-record edits use per-file `QSaveFile`; multi-file updates back up affected existing files beside the playlist and attempt rollback if a write or removal fails. Errors report whether the previous state was restored and recommend checking the SD card. This rollback handles detected failures, not sudden power loss or process termination during a batch.

- Removing songs renumbers the remaining suffix contiguously from the unchanged prefix; PlaylistStorage writes only records whose serialized content or numbered filename changed and removes obsolete files after the replacement records are written.
- `insertRows()` persists from the insertion row through the affected suffix; `moveRows()` persists only the interval between source and destination. The storage API receives this row interval and does not rewrite records outside it.
- Multi-file save backups are kept in the host's temporary directory, outside PLAYLIST, so an interrupted operation cannot leave a temporary backup directory that playlist discovery mistakes for a user playlist.
- If multi-file rollback cannot fully restore the previous playlist, the temporary backup is retained instead of being deleted, and its filesystem path is included in the error shown to the user for possible manual recovery.




QML registration note: classes marked with `QML_ELEMENT` and registered as creatable QML types must not be declared `final`, because Qt generates a `QQmlElement<T>` subclass for construction.


## Automated playlist and audio tests (Qt Test)
The `playlist_tests` target covers playlist storage, repository, service, and Qt Model/View behavior. The `audio_tests` target covers AudioMixer behavior with in-memory QBuffer streams and DecodeStream lifecycle using generated PCM WAV fixtures: decoder starvation, Track 2-only playback, shorter-stream draining, channel/volume/mute routing, signed-16-bit clipping, frame-aligned reads, spool cursor accounting, seek-ahead during decoding, synchronized two-track seeking, consumed position, the one-hour PCM spool limit, missing-source handling, stop/restart session isolation, invalid playback-mode rejection, output/decode errors, and decoder-error completion semantics. DecodeStream tests use Qt's decoder backend but do not require a physical audio output device. QAudioSink hardware integration and QML automation remain manual integration checks.

Build and run all three suites:

```sh
cmake -S . -B build-tests -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH="/path/to/Qt/6.7.x/macos"
cmake --build build-tests --target playlist_tests audio_tests soundcore_tests
ctest --test-dir build-tests --output-on-failure
```

Mixer tests use QBuffer fixtures; decoder lifecycle tests use generated PCM WAV files and do not construct QAudioSink. The `soundcore_tests` target exercises SoundCore integration with PlaylistModel and SdContentModel proxy indexes, source switching, PlayPause model identity, SD selection-path synchronization, QML-facing playback-state and playback-position properties, invalid playlist requests preserving an active source, explicit stop reset, and metadata cleanup after decoder failure or natural completion through a fake output. Physical output-device integration and full QML automation still require a separate platform integration setup.
