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
**Playlists:** QML -> UiCore -> PlaylistService -> PlaylistModel / PlaylistRepository. PlaylistService coordinates playlist lifecycle and model construction. It validates one playlist by its slash-separated relative path, using song records loaded from PlaylistRepository/PlaylistStorage and the service-owned workspace root; validation must not query PlaylistModel roles or indexes. PlaylistRepository recursively discovers playlist directories and safely creates/removes slash-separated paths relative to PLAYLIST/. PlaylistModel owns per-playlist song state and Qt Model/View behavior; PlaylistStorage reads and writes numbered .ego files. Keep Qt Model/View concerns out of storage and repository code, and keep filesystem lifecycle logic and playlist validation out of UiCore.

**Nested playlist directory rules:**
- A playlist path is a slash-separated relative path under PLAYLIST/, e.g. Collections/Rock. It is the stable playlist identifier; the final path component is the display name.
- Every directory below PLAYLIST/ is exposed as a playlist. A directory may itself contain .ego files and child playlist directories.
- Existing top-level playlists remain valid without migration. Their identifiers remain their directory names.
- Absolute paths, empty path components, dot/dot-dot components, hidden components, drive-qualified paths, backslashes, control characters, and symbolic-link traversal are rejected.
- PlaylistModel receives the workspace path explicitly; it must not infer it by walking upward from its playlist directory because playlists may be nested.
- The playlist creation dialog accepts slash-separated relative paths. Removing a directory removes its entire nested subtree.

**Audio import:** Qt is the primary conversion path. On failure, source data and previous destination assignment must remain available.
**Playback:** check decoder/stream ownership, track switching, stop, errors, teardown, and buffering of long files.

## Previous static-audit areas
These are areas to re-check, not confirmed defects in current HEAD:
- PlaylistModel index bounds and move semantics.
- Failure-safe multi-file .ego updates and rollback.
- Preserving previous destination on failed import.
- Import queue terminal state after errors.
- Player/decoder lifetime and memory growth.
- Remaining UiCore/SdContentModel responsibility split (workspace policy, validation, localization, media import).
- Tests and CI on minimum OS versions.
