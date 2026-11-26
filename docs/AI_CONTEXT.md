# EgoApp Project Context

EgoApp is AMT's desktop application for AMT EgoGig4 users. It manages playlists and prepares media on an SD card or local folder for later transfer.

## Constraints
- User-data preservation is critical.
- macOS: Qt 6.7, minimum macOS 11.
- Windows: Qt 6.11/6.12, minimum Windows 10 1809.
- Linux: Qt 6.11/6.12; minimum Ubuntu 22.04.
- Primary audio conversion uses Qt.
- SoX converter is legacy; do not modify third-party SoX library internals.

## Source map
Verify paths against the current tree.
| Area | Files |
|---|---|
| Playlist feature | `MainApplication/playlists/playlistmodel.cpp/.h`, `playlistrepository.cpp/.h`, `playliststorage.cpp/.h`, `song.cpp/.h` |
| SdContentModel | `MainApplication/sdcontentmodel.cpp/.h` |
| UiCore | `MainApplication/uicore.cpp/.h` |
| Player | `MainApplication/soundplayer.cpp/.h` |
| Decoder | `MainApplication/decodestream.cpp/.h` |
| QML UI | `MainApplication/qml/*.qml`, compiled as the `EgoApp` module |
| Entry point | `MainApplication/main.cpp` |
| Legacy resampler | `WavConverterLib/resampler.cpp/.h` |
| Legacy shell | `WavConverterShell/CMakeLists.txt` |

## Data paths
**Playlists:** QML → UiCore → PlaylistModel; PlaylistRepository discovers playlist directories, while PlaylistStorage reads and writes numbered `.ego` files. Keep Qt Model/View concerns out of storage and repository code.
**Audio import:** Qt is the primary conversion path. On failure, source data and previous destination assignment must remain available.
**Playback:** check decoder/stream ownership, track switching, stop, errors, teardown, and buffering of long files.

## Previous static-audit areas
These are areas to re-check, not confirmed defects in current HEAD:
- PlaylistModel index bounds and move semantics.
- Failure-safe multi-file `.ego` updates and rollback.
- Preserving previous destination on failed import.
- Import queue terminal state after errors.
- Player/decoder lifetime and memory growth.
- Remaining UiCore/SdContentModel responsibility split (workspace policy, validation, localization, media import).
- Tests and CI on minimum OS versions.
