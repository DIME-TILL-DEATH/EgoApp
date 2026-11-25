# EgoApp Architecture

*Working overview; verify against the current implementation.*

```mermaid
flowchart TD
 QML[QML UI] --> CORE[UiCore]
 CORE --> PL[PlaylistModel]
 CORE --> SD[SdContentModel]
 PL --> FS[Filesystem / .ego]
 SD --> FS
 CORE --> IMP[Qt audio import / conversion]
 IMP --> FS
 QML --> PLAYER[SoundPlayer]
 PLAYER --> DEC[DecodeStream]
 DEC --> OUT[Qt audio output]
 LEGACY[Legacy SoX converter] -. isolated legacy .-> FS
```

This is a conceptual diagram, not a complete call graph.

## Responsibility areas
- QML: presentation and lightweight coordination.
- UiCore: QML/C++ bridge; should not absorb all business logic.
- PlaylistModel: playlist state and Qt Model/View contracts.
- SdContentModel: selected media/folder contents.
- SoundPlayer/DecodeStream: playback, decoding, and resource ownership.
- Qt audio conversion: primary audio-preparation path.
- SoX: isolated legacy component, not the primary audio path; do not modify third-party internals.

## Invariants
1. Failed operations must not destroy user source data.
2. Do not truncate the old file before the replacement is ready.
3. Propagate errors; do not report partial success as full success.
4. Keep model data and signals consistent.
5. Keep complex file operations out of QML.
6. Make async-object and temporary-file lifetimes explicit.

## File operations
Prefer a temporary file in the destination directory, validate it, then safely replace the old file. Use `QSaveFile` where its semantics fit. Handle SD removal, access errors, low disk space, partial writes, and rename failures. Preserve the previous destination until imported output is validated.

## Errors and tests
Define running/success/failure/cancelled states, partial-success behavior, cleanup, model updates, and actionable diagnostics. Test filesystem failures, index boundaries, repeated playback, and cancellation.

## Open questions
UiCore/model responsibility boundaries; synchronous versus asynchronous operations; stable `.ego` serialization contract; existing test and CI coverage.
