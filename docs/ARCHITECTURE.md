# EgoApp Architecture

*Working overview; verify against the current implementation.*

```mermaid
flowchart TD
 QML[QML UI in MainApplication/qml] --> CORE[UiCore]
 CORE --> SERVICE[PlaylistService]
 CORE --> SD[SdContentModel]
 SERVICE --> PL[PlaylistModel]
 SERVICE --> REPO[PlaylistRepository]
 PL --> STORAGE[PlaylistStorage]
 REPO --> STORAGE
 STORAGE --> FS[Numbered .ego files]
 SD --> FS
 CORE --> IMP[Qt audio import / conversion]
 IMP --> FS
 QML --> PLAYER[SoundPlayer]
 PLAYER --> DEC[DecodeStream]
 DEC --> OUT[Qt audio output]
 LEGACY[Legacy SoX converter] -. isolated legacy .-> FS
```

This is a conceptual diagram, not a complete call graph.

## Source layout
- MainApplication/qml/: QML presentation files compiled into the EgoApp module.
- MainApplication/: C++ entry point, bridge objects, media model, and audio implementation.
- MainApplication/playlists/: playlist feature module (PlaylistService, PlaylistModel, PlaylistRepository, PlaylistStorage, and Song). QML-facing type names and the .ego file format remain unchanged.
- MainApplication/icons/, translations/, and docs/: runtime resources; their resource paths remain unchanged.

## QML module and runtime loading
CMake registers QML sources from MainApplication/qml/ and assigns each source its basename through QT_RESOURCE_ALIAS. This preserves public QML type names and the AppGlobals singleton. The MainApplication/playlists/ directory is also added to the application target's private include paths because Qt's generated QML type-registration source includes registered C++ headers by basename. Runtime loading remains QQmlApplicationEngine::loadFromModule("EgoApp", "Main"); the application does not load QML from a source-tree filesystem path.

## Responsibility areas
- QML: presentation and lightweight coordination.
- UiCore: QML-facing application facade; owns UI state and translates application results into Qt signals. Its constructor only establishes internal signal connections. The root QML component calls `initialize()` from `Component.onCompleted`; only then does UiCore configure the initial workspace, load playlists, and configure the application language. After the workspace and playlist models are ready, UiCore emits `workspaceChanged()` to notify QML bindings to `workspacePath` and `avaliablePlaylists`. Playlist load diagnostics are delivered directly by signal because QML connections are ready before initialization starts. UiCore does not retain workspace-directory state: the workspace path is passed to SdContentModel and PlaylistService, while PlaylistService is the authoritative source for the active workspace path. UiCore does not own playlist discovery or directory lifecycle logic.
- PlaylistService: coordinates playlist discovery, creation/removal, model construction, and model lifetimes for the active workspace. It also validates an individual playlist by its stable relative path, loading song records through PlaylistRepository/PlaylistStorage and resolving media paths against its workspace root; validation does not inspect PlaylistModel roles or indexes. It is independent of QML and exposes the playlist models to UiCore.
- PlaylistRepository: recursively discovers playlist directories and safely creates/removes slash-separated relative paths under PLAYLIST/. It returns plain PlaylistData records and has no QML or Qt item-model dependency.
- PlaylistStorage: reads and writes numbered .ego files and preserves their existing two-line format (including the optional > marker). The .ego format is immutable for this work.
- PlaylistModel: one playlist’s song state and Qt Model/View contracts. It delegates persistence to PlaylistStorage and does not implement file-format parsing or filesystem discovery. It receives the workspace root explicitly so media links resolve correctly regardless of playlist nesting depth.
- SdContentModel: selected media/folder contents.
- SoundPlayer/DecodeStream: playback, decoding, and resource ownership.
- Qt audio conversion: primary audio-preparation path.
- SoX: isolated legacy component, not the primary audio path; do not modify third-party internals.

## PlaylistModel Qt Model/View contract
- PlaylistModel is a flat list: `rowCount(parent)` returns zero for a valid parent index.
- `moveSong(from, to)` accepts final row indexes. Internally it translates the target into Qt's `moveRows()` destination-child insertion position, which may equal the pre-move row count when moving an item to the end. `moveRows()` rejects overlapping/no-op destinations and uses Qt's required pre-removal destination-child semantics.
- Updates to one song emit `dataChanged()` with the exact affected roles: PlayNext changes only PlayNextRole; track-link changes notify the path, name, missing-state and (for track 1) MIDI-existence roles.
- `insertRows()` rejects positions outside `[0, rowCount()]` and counts that exceed the representable row range; `removeRows()` checks the requested count against the rows remaining after the starting position, avoiding overflow in `position + count`.
- `moveSong()` returns whether the persistent source-model move succeeded. The QML drag proxy reverts its optimistic visual reorder on failure, and song deletion changes the selection only after a successful removal.

## Nested playlist directories
A playlist's stable identifier is its path relative to PLAYLIST/, not its basename. For example, Collections/Rock and Live/Rock are distinct playlists even though both have the display name Rock. The backend exposes every directory below PLAYLIST/ as a playlist; directories can contain both their own numbered .ego files and child directories. Existing top-level directories continue to use their existing names as identifiers.

Repository operations reject traversal and symbolic-link components and verify canonical path containment before recursively deleting a playlist directory. Removing a parent directory removes its complete nested subtree. Playlist directory nesting does not change the location, naming, content, or serialization of .ego files.

## Invariants
1. Failed operations must not destroy user source data.
2. Do not truncate an existing .ego file before its replacement is ready; writes use QSaveFile.
3. Propagate file errors and do not report stale-file cleanup failure as success.
4. Keep model data and signals consistent.
5. Keep complex file operations and playlist validation out of QML and Qt item models.
6. Make async-object and temporary-file lifetimes explicit.
7. Never change the existing .ego format when implementing playlist features.

## File operations and known limitation
Each individual .ego replacement is protected by QSaveFile. A playlist update spans multiple files, however, so a failure partway through the batch can still leave a mixed on-disk generation. A subsequent hardening step should stage the complete generation and provide rollback/commit semantics before removing stale files. Do not describe the current batch operation as atomic.

## Errors and tests
Test file-open/read/write/commit failures, malformed and duplicate .ego filenames, out-of-range numbers, invalid UTF-8, incomplete records, stale-file removal failures, empty playlists, numeric file ordering, model index boundaries, reorder boundaries, and cancellation. Nested repository tests should cover recursive discovery, duplicate basenames at different depths, old top-level paths, invalid traversal, symlink rejection, and nested create/remove behavior. The current repository does not yet have dedicated automated tests for the playlist persistence layer. Model contract tests for flat parent handling, reorder boundaries and dataChanged role lists are deferred until the playlist architecture work is complete.

## Open questions
Further UiCore extractions: workspace policy/settings, playlist validation, localization, and external application launching.
