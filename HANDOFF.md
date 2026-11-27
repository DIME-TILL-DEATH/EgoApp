# EgoApp — Engineering Handoff

> **Purpose:** persistent checkpoint for continuing the engineering plan across work sessions.
> This file records the current state, evidence, dependencies, and the next recommended action. It is not a substitute for source code, tests, or detailed design documents.

**Last verified:** 2026-11-27  
**Tracking baseline:** `dev` at `a3307ef71789e53ffc5679d9d052f75e3bc424c2` (after PR #5 squash merge; T6 work is isolated on a temporary branch)  
**Current plan:** architecture baseline → audio test protection → SdContentModel audit → UiCore audit → integration tests → CI hardening

## How to use this file

1. Read this file before starting work on the six workstreams below.
2. Verify the current branch and repository state; this file is a checkpoint, not proof that the repository has not changed since the date above.
3. Work on one logical subtask at a time. For a new feature or behavior change, follow TDD: add or adjust a failing test first, implement the smallest change, then run the relevant suite and broader regression tests.
4. Update this file in the same logical change as implementation or test changes:
   - update the status and evidence;
   - check off only items that are actually complete;
   - record newly discovered risks, decisions, and deferred work;
   - refresh **Next action** so another session can continue without reconstructing the history.
5. Do not mark a task complete because code exists. Completion requires its acceptance criteria and verification evidence.
6. Keep detailed component contracts in `docs/ARCHITECTURE.md`, project-specific implementation context in `docs/AI_CONTEXT.md`, and platform claims in `docs/PLATFORM_SUPPORT.md`. This file coordinates work; it should not duplicate those documents.

### Status vocabulary

- **NOT STARTED** — no focused work has begun.
- **IN PROGRESS** — active work is underway; acceptance criteria remain.
- **PARTIAL** — useful implementation exists, but an explicit gap or dependency remains.
- **BLOCKED** — cannot proceed until a named dependency or decision is resolved.
- **DONE** — acceptance criteria are met and evidence is recorded.

Use these labels consistently. A checked box means the specific subtask is complete, not merely investigated.

## Current status at a glance

| ID | Workstream | Status | Current outcome |
|---|---|---|---|
| T1 | Record and reconcile current architecture | **DONE** | Platform baseline, component responsibilities, test inventory, agent rules, and documentation-maintenance policy reconciled on `dev` |
| T2 | Complete audio-core test protection | **PARTIAL** | Playlist fixture correction is merged; all three test targets pass on `dev`; remaining SoundPlayer transition coverage is still open |
| T3 | Audit SdContentModel | **NOT STARTED** | Responsibility and failure-mode audit remains |
| T4 | Audit UiCore | **NOT STARTED** | Facade boundary and application-flow audit remains |
| T5 | Add integration tests | **PARTIAL** | C++ SoundCore integration tests exist; QML and broader workspace/import/playlist integration remain |
| T6 | Establish CI and build validation | **IN PROGRESS** | CI workflow is merged and active on `dev`; post-merge three-platform CI is green; Release validation remains |

---


## Repository history and branch policy

**Policy adopted:** 2026-11-27. Preserve the existing history of the two long-lived branches; do not rewrite either branch. The current branch heads were recorded before this documentation update:

| Branch | Verified head before this change | Role |
|---|---|---|
| `dev` | `d4a1c079e875388ece9fd7e9c23470f0a8757c95` | Active integration/development branch |
| `master` | `56f1c5cdc315d9848c7f778c3d822d729d230c60` | Release branch; preserve independently |

These SHAs are historical control points, not branch names or instructions to reset either branch. Updating this handoff creates a normal new commit on `dev`; it does not amend, rebase, or force-update either existing history.

### Branch workflow

- Keep `dev` and `master` as the only long-lived branches.
- Create feature branches from the current `dev` only when work needs isolation or a reviewable unit.
- Keep feature branches temporary. Open a pull request targeting `dev`; after review and validation, use **Squash and merge**.
- Delete a feature branch after its squash-merged changes are verified on `dev`. Do not retain completed work branches as parallel development histories.
- Do not use merge commits or rebase/force-push to rewrite the history of `dev` or `master`.
- Do not delete a branch solely because it is old or absent from the current workflow. First verify its unique commits and confirm that all intended content is present on `dev` or deliberately archived.
- Keep `master` separate from `dev` unless a release operation explicitly calls for a reviewed integration.

### Current branch refs

At the start of T6, the verified long-lived heads were `dev=a3307ef71789e53ffc5679d9d052f75e3bc424c2` and `master=56f1c5cdc315d9848c7f778c3d822d729d230c60`. Temporary branches currently include `docs/t1-documentation` (PR #5 is merged; branch cleanup remains) and `ci/t6-release-build` (active T6 implementation). Delete completed temporary branches after their intended changes are verified on `dev`.

Keep only `dev` and `master` as long-lived branches. Do not introduce a permanent `pre-release` branch: Release validation is scoped to PRs whose base is `master` and pushes to `master`.

## T1 — Record and reconcile the current architecture

**Status:** DONE  
**Verified baseline:** `dev` at `d4a1c079e875388ece9fd7e9c23470f0a8757c95` (before the documentation branch commit).

### Completed

- [x] Reconciled platform claims: Ubuntu 22.04 is the only claimed Linux baseline; macOS 11 / Qt 6.7 and Windows 10 1809 / Qt 6.11–6.12 remain target requirements with validation caveats.
- [x] Corrected the README Linux row; removed unsupported Ubuntu 24.04 and Debian 12 claims.
- [x] Verified the architecture flow against current source and CMake: QML → UiCore/SoundCore; UiCore → PlaylistService/SdContentModel; playlist storage/repository/model; SD model and Qt decoder; SoundCore/audio playback pipeline.
- [x] Documented UiCore and SdContentModel responsibilities and their current boundaries without claiming that their audits/refactors are complete.
- [x] Reconciled the documented automated test inventory with the existing three Qt Test targets and the Debug-only CI matrix. Hardware, QML automation, minimum-OS runtime, and release packaging remain separate validation areas.
- [x] Updated AGENTS.md and the agent-specific instruction files to require documentation and HANDOFF updates in the same coherent change.
- [x] Updated branch references and status information to the current repository state.

### Evidence and limitations

- Repository documentation inventory: README.md, AGENTS.md, CLAUDE.md, GEMINI.md, QWEN.md, HANDOFF.md, and the three files under docs/.
- Current branch refs at verification: `dev=d4a1c079e875388ece9fd7e9c23470f0a8757c95`, `master=56f1c5cdc315d9848c7f778c3d822d729d230c60`; no other long-lived branches were present.
- CI currently builds/runs the three test targets in Debug on Ubuntu 22.04 / Qt 6.12.1, macOS 15 Intel / Qt 6.7.3, and Windows Server 2022 / Qt 6.12.1. It does not build the full application in Release.
- Documentation-only change; no source or runtime tests are required for this workstream. Documentation content and GitHub refs are rechecked after commit.

---
## T2 — Complete the audio-core test protection

**Status:** PARTIAL  
**Relevant code/tests:** `MainApplication/audio/`, `MainApplication/tests/audio_tests.cpp`, `MainApplication/tests/soundcore_tests.cpp`

### Already implemented

- [x] AudioMixer tests cover synchronized reads, short/finished streams, channel/volume/mute routing, clipping, frame alignment, position accounting, and transactional seek/read rollback.
- [x] DecodeStream tests cover generated PCM fixtures, spool limits and cleanup, seek-ahead, cursor accounting, stop/restart session isolation, and decoder terminal errors.
- [x] SoundPlayer tests use an injected fake AudioOutput; no physical audio device is required. Existing cases cover invalid modes, Track 2-only playback, delayed data arrival, pause/resume, stop/restart, output/decode errors, short-track drain, and output-buffer drain.
- [x] SoundCore tests cover model/index validation, source switching, QML-facing state/progress, invalid requests preserving active playback, and metadata cleanup after error/completion.

### Remaining work

- [x] Merge the playlist test-fixture correction from PR #4 into `dev` (squash merge `ff6f4321b6d9d26dc8efa1b736bec8b416c06c0c`).
- [x] Re-run all three Qt Test targets on updated `dev`; run `45973266798` passed on commit `f3241bc04eb545154e902cd3f9dbb21aee7b588d`.
- [ ] Review remaining SoundPlayer transition/error cases against the state machine; add tests only for uncovered behavior rather than duplicating existing cases.
- [ ] Keep production QAudioSink adapter/hardware behavior explicitly classified as platform integration testing, not as covered by the fake output tests.

### Acceptance criteria

- [x] All three Qt Test targets pass on `dev`.
- [ ] SoundPlayer state transitions and terminal paths have deterministic hardware-independent coverage.
- [ ] SoundCore-facing playback metadata/state is verified for normal completion, stop, source switch, and terminal errors.
- [ ] Any untested hardware/QML behavior is documented as a manual or separate integration check.

### Current evidence and dependency

The three test targets passed on PR #4 head `36b0c1462d7ef6e4ea3a6bb3f159a5a11eae4c31` (2026-11-27), on Ubuntu 22.04 / Qt 6.12.1, macOS 15 Intel / Qt 6.7.3, and Windows Server 2022 / Qt 6.12.1. The successful Pull Request workflow run is [45971998479](https://github.com/DIME-TILL-DEATH/EgoApp/actions/runs/45971998479). PR #4 was squash-merged into `dev` as `ff6f4321b6d9d26dc8efa1b736bec8b416c06c0c`.

The post-merge Push workflow run [45973266798](https://github.com/DIME-TILL-DEATH/EgoApp/actions/runs/45973266798) completed successfully on `dev` commit `f3241bc04eb545154e902cd3f9dbb21aee7b588d`: all three targets passed on Ubuntu 22.04 / Qt 6.12.1, Windows Server 2022 / Qt 6.12.1, and macOS 15 Intel / Qt 6.7.3. An earlier post-merge run was superseded by the HANDOFF-only documentation update; this successful run includes that update. The fixture correction fixes a pre-existing assumption in `repositoryCreatesDiscoversAndRemovesNestedPlaylists()`: the shared test setup creates a default `PLAYLIST/Test` directory, while the repository test expected only the three directories it created.

The CI cleanup also updates `actions/checkout` and `actions/upload-artifact` to Node.js 24-compatible v7 releases, replaces the machine-specific absolute SoX symlink with a sibling-relative link, removes a noisy warning for unsupported PlaylistModel roles (and adds a regression test), replaces a deprecated QDateTime constructor, and installs the Linux VA-API / PipeWire runtime libraries used by Qt Multimedia tests.

One non-blocking CMake warning remains on Qt 6.12.1: Qt's optional `Qt6QmlAssetDownloaderPrivate` plugin dependency lookup cannot find the separate `Qt6TaskTree` module. The application does not use TaskTree; the module is a Qt technology preview. Expected FFmpeg warnings from tests that intentionally open invalid media are retained because they verify decoder-error paths.

---

## T3 — Audit SdContentModel

**Status:** NOT STARTED  
**Relevant code:** `MainApplication/sdcontentmodel.cpp/.h`, related proxy/model classes, WAV import/conversion code, and their callers.

### Preliminary observations (source inspection only; not yet confirmed by focused tests)

- The current import path opens/creates the destination WAV before asynchronous decoding has succeeded; opening an existing destination with `WriteOnly` may truncate it. Confirm the exact behavior and add a regression test before changing it.
- `slBufferReady()` writes decoded blocks without checking the returned byte count; final header/copy operations and MIDI copy results also need failure-path review.
- Decoder errors are currently forwarded as a signal; verify cleanup and terminal queue/state behavior.

These are audit leads, not completed findings or evidence that a specific user-data-loss scenario has been reproduced.

### Work items

- [ ] Map every responsibility and public entry point: directory enumeration, filtering, selection, file operations, import, WAV conversion, and model notifications.
- [ ] Trace each operation from QML/UiCore to filesystem/converter and back. Identify duplicated policy and hidden cross-layer dependencies.
- [ ] Audit filesystem failure handling: unreadable/missing workspace, SD removal, partial reads/writes, rename/remove failures, low disk space, cancellation, and stale indexes.
- [ ] Audit WAV import/conversion and destination assignment. Preserve source files and previous assignments on failure; define batch partial-success behavior.
- [ ] Identify cohesive extraction boundaries only after the responsibility and data-flow map is complete. Avoid a broad refactor before the audit has evidence.
- [ ] Add/adjust tests before changing behavior. Prefer injectable filesystem/conversion seams where they materially improve deterministic tests.

### Acceptance criteria

- [ ] A responsibility map identifies what stays in SdContentModel and what belongs elsewhere.
- [ ] Each significant file/import operation has documented success, failure, cancellation, and partial-success semantics.
- [ ] Risks and extraction candidates are prioritized and justified.
- [ ] Tests cover critical error paths and preserve user data.
- [ ] Any proposed refactor has a staged migration plan and does not change unrelated behavior.

---

## T4 — Audit UiCore

**Status:** NOT STARTED  
**Relevant code:** `MainApplication/uicore.cpp/.h`, QML callers, settings/localization code, process-launching code, PlaylistService, SdContentModel, SoundCore.

### Work items

- [ ] Inventory QML-facing properties, invokable methods, signals, and constructor/initialization responsibilities.
- [ ] Trace workspace selection/initialization, settings persistence, localization, external process launch, playlist orchestration, import orchestration, and error presentation.
- [ ] Decide what belongs in the facade (UI/application coordination) versus dedicated services/policies.
- [ ] Audit async process lifetime, startup ordering, signal connection timing, error/exit handling, and cleanup.
- [ ] Identify application scenarios suitable for deterministic tests; introduce seams only where needed.
- [ ] Add tests before behavior changes and update architecture/context docs alongside any extraction.

### Acceptance criteria

- [ ] Every public UiCore API has a documented purpose and caller.
- [ ] UiCore has a clear facade boundary and does not own lower-level filesystem/domain logic unnecessarily.
- [ ] Workspace/settings/localization/process responsibilities and their ownership are explicit.
- [ ] Important startup, failure, and external-process scenarios have automated tests or a documented reason they remain manual.
- [ ] No QML-facing contract changes without corresponding QML/C++ regression coverage.

---

## T5 — Add integration tests

**Status:** PARTIAL  
**Relevant code/tests:** `MainApplication/tests/`, QML module and application registration in CMake.

### Existing coverage

- [x] C++ SoundCore integration tests exercise SoundCore with PlaylistModel and SdContentModel/proxy indexes and a fake audio output.
- [x] Playlist tests cover storage/repository/service/model behavior in the Qt Test target.

### Work items

- [ ] QML ↔ C++: test root initialization, property/signal bindings, model exposure, and critical playback/playlist interactions.
- [ ] Workspace ↔ filesystem: test workspace selection, initialization, missing/unreadable media roots, and recovery after media removal.
- [ ] WAV import ↔ track assignment: test success, conversion failure, cancellation, destination preservation, and source preservation.
- [ ] Playlist ↔ SoundCore ↔ SoundPlayer: test a representative end-to-end playback flow, source switching, stop, completion, and error propagation.
- [ ] Define a practical test pyramid: fast unit tests by default, focused integration tests in CI, and hardware/platform checks in a separate validation layer.
- [ ] Keep test fixtures isolated and deterministic; never rely on a pre-existing developer workspace or physical SD card.

### Acceptance criteria

- [ ] Each listed cross-component flow has at least one meaningful automated integration test, or a specific limitation and manual procedure is recorded.
- [ ] Tests run in a clean checkout and do not depend on host-specific paths, existing user files, or physical audio hardware.
- [ ] Failures identify the layer and scenario clearly.
- [ ] Integration tests are included in the documented CI test inventory.

---

## T6 — Establish CI and build validation

**Status:** IN PROGRESS  
**Relevant workflow:** `.github/workflows/ci.yml` (merged into `dev` by PR #4 squash merge).

### Work items

- [x] Define a cross-platform Qt test matrix and pin explicit Qt patch versions.
- [x] Build `playlist_tests`, `audio_tests`, and `soundcore_tests`; run them through CTest.
- [x] Preserve configure/build/test logs as workflow artifacts.
- [x] Run on Push and Pull Request.
- [x] Fix the first CI-discovered Linux runtime dependency, Windows resource-file configuration, and playlist test-fixture issue on the CI branch.
- [x] Verify green CI on Ubuntu 22.04, macOS 15 Intel, and Windows Server 2022.
- [x] Review and squash-merge PR #4 into `dev`.
- [x] Confirm post-merge Push workflow run `45973266798` is green on `dev`.
- [x] Add a build-only Release configuration for the full Windows and Linux application, gated to pull requests targeting `master` and pushes to `master`.
- [x] Add the top-level `EGOGIG_ENABLE_RELEASE_PACKAGING` switch (default `ON`) and gate deployment/packaging post-build hooks in MainApplication, WavConverterShell, and WavConverterLib.
- [x] Keep macOS Release signing and package generation local-only; CI does not receive signing credentials.
- [x] Upload Release configure/build logs as 14-day artifacts. Do not publish installers or bundles in the initial validation stage.
- [x] Keep minimum-OS runtime, physical audio/SD-card, Windows 10 1809, and local macOS signing/package checks separate.

### Acceptance criteria

- [x] CI is active on `dev` for Push and Pull Request.
- [x] All three current test targets build and pass on Ubuntu 22.04, macOS 15 Intel, and Windows Server 2022.
- [x] Debug tests remain on the three-platform matrix; full-app Release builds are configured for Windows/Linux only on PRs targeting `master` and pushes to `master`.
- [ ] Both Release matrix legs pass on the PR-to-`master` acceptance run.
- [ ] Release CI builds the application without invoking Inno Setup, cqtdeployer/dpkg, macdeployqt, codesign, or productbuild.
- [x] macOS Release signing/package validation remains a documented local owner step.
- [x] Logs are retained as 14-day artifacts and are retrievable after a failure.
- [x] CI claims distinguish hosted-runner build validation from actual runtime validation on minimum OS versions.

### Platform caveat

The requested Windows 10 hosted runner is not available in the selected GitHub-hosted matrix. Windows Server 2022 validates the Windows/MSVC build and tests, but does not prove runtime compatibility with Windows 10 1809. A Windows 10 self-hosted runner or clean VM is still needed for that claim.

### T6 implementation evidence (pending first Release run)

- Added `release-build` to `.github/workflows/ci.yml`; it is gated to PRs with base `master` and pushes to `master`, with Ubuntu 22.04 and Windows Server 2022 / Qt 6.12.1 matrix legs.
- The job configures the full project with `BUILD_TESTING=OFF`, `CMAKE_BUILD_TYPE=Release`, and `EGOGIG_ENABLE_RELEASE_PACKAGING=OFF`, then builds all default targets. Configure/build logs are uploaded for 14 days.
- Added the packaging switch in the root CMake project and guarded app, converter-shell, and converter-library deployment/signing/installer hooks.
- Updated `docs/PLATFORM_SUPPORT.md` and `docs/AI_CONTEXT.md`. This is implementation evidence only; the new workflow must still be exercised by a PR targeting `master` before T6 can be marked DONE.

---

## Cross-cutting engineering rules

- Documentation is part of every implementation: update all affected docs, agent rules, and this handoff in the same coherent change. Never leave known documentation drift for a later task.

- Preserve user data above convenience or speed. Never destroy source data or the previous destination on a failed operation.
- Keep the existing numbered `.ego` file format and workspace-rooted media-path semantics backward compatible.
- Keep Qt Model/View contracts correct; do not retain transient proxy indexes across model changes.
- Make async ownership, cancellation, terminal errors, and cleanup explicit.
- Keep QML focused on presentation and lightweight coordination.
- Do not extract classes solely to reduce file size. Establish responsibility boundaries from an audit and tests.
- New behavior follows TDD: failing test → minimal implementation → focused suite → regression suite.
- Run Debug and Release checks when changes can affect resource lifetime, build configuration, or packaging.
- Update `docs/ARCHITECTURE.md`, `docs/AI_CONTEXT.md`, and `docs/PLATFORM_SUPPORT.md` when their respective facts change.
- Prefer one commit per coherent group of changes, not one commit per mechanical step.

## Next action

1. Open a validation PR from `ci/t6-release-build` to `master` so the new Release matrix actually runs. Iterate on CMake/workflow failures until both Ubuntu and Windows Release builds pass.
2. After both Release jobs pass, close the validation-only PR without merging into `master`; open a PR from the same feature branch to `dev` and squash-merge it automatically. Then remove the temporary feature branch and refresh this handoff with the final run/merge SHAs.
3. Resume T2: compare the SoundPlayer state machine with existing fake-output tests and add only missing deterministic transition/error cases, then proceed to T3 and T4/T5.

## Tracking sources and conventions

This handoff format uses a small, repository-local checklist because the current plan has six connected workstreams and needs to remain readable without requiring a separate project-management service. The structure follows GitHub's documented practices: break work into trackable tasks, make progress/status visible, and use issues/projects for finer-grained ownership, priority, iteration, and dependency tracking when needed.

- [GitHub: Planning and tracking work](https://docs.github.com/en/issues/tracking-your-work-with-issues/learning-about-issues/planning-and-tracking-work-for-your-team-or-project)
- [GitHub: About tasklists](https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting/about-tasklists)
- [GitHub: About Projects](https://docs.github.com/en/issues/planning-and-tracking-with-projects/learning-about-projects/about-projects)
- [GitHub: Best practices for Projects](https://docs.github.com/en/issues/planning-and-tracking-with-projects/learning-about-projects/best-practices-for-projects)
