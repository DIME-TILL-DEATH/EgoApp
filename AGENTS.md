# Instructions for AI Agents

Before working, read `docs/AI_CONTEXT.md`. For architecture work, read `docs/ARCHITECTURE.md`; for build/platform work, read `docs/PLATFORM_SUPPORT.md`. Verify documentation against the current source tree.

## Priorities
1. Preserve user data.
2. Correct behavior and predictable error handling.
3. Compatibility with older target OS and Qt versions.
4. Maintainability.
5. Performance and usability.
Never trade data integrity for convenience or speed.

## Platform constraints
- macOS: Qt 6.7, minimum macOS 11.
- Windows: Qt 6.11/6.12, minimum Windows 10 1809.
- Linux: Qt 6.11/6.12, minimum OS Ubuntu 22.04.
Avoid APIs unavailable on the minimum target unless a justified fallback exists. Test on the minimum OS.

## Audio and SoX
The primary audio conversion path uses Qt. Treat the SoX converter as legacy. Do not assume SoX is the primary path and do not modify third-party SoX library internals.

## Safe file operations
- Never truncate an existing user file before the replacement is ready.
- Prefer a temporary file beside the destination, validate it, then safely replace the old file.
- Consider `QSaveFile` where appropriate.
- Check open/read/write/close/rename/remove failures.
- Account for partial writes, low disk space, SD removal, and lost access.
- Report success only after completion.
- Do not remove source data before validating output.
- Preserve the previous destination/assignment when import fails.
- Define partial-success behavior for batch operations.

## Qt Model/View and QML
Follow `QAbstractItemModel` contracts, matching `begin...`/`end...` calls, valid indexes, and consistent roles. Check bounds before access. Keep complex business and filesystem logic out of QML. Test reorder at boundaries, self-moves, range moves, and empty models.

## Lifetimes and audio
Use RAII and explicit ownership. Async operations need success/failure/cancelled states and cleanup on every path. Check decoder/player lifetimes, repeated playback, track changes, and buffer growth. Avoid loading large files wholly into memory when streaming is practical.

## Source layout
- Keep QML presentation files in `MainApplication/qml/`, separate from C++.
- Preserve QML file basenames through `QT_RESOURCE_ALIAS` and register `AppGlobals.qml` as a singleton.
- Load with `QQmlApplicationEngine::loadFromModule("EgoApp", "Main")`; never switch to source-relative runtime loading.
- Keep C++ files in their current location until focused module boundaries are defined.

## Workflow
Trace data flow before editing; keep changes local; add tests; test Debug/Release for resource-lifetime changes; update docs. Report changes, checks, and limitations.

## Checklist
- [ ] Source data survives failures.
- [ ] Filesystem errors and media removal are handled.
- [ ] Index bounds and edge cases are checked.
- [ ] Object ownership/lifetimes are sound.
- [ ] Minimum OS/Qt compatibility is preserved.
- [ ] Tests added or omission explained.
