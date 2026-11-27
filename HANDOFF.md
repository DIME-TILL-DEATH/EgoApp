# EgoApp — Engineering Handoff

> **Purpose:** persistent checkpoint for continuing the engineering plan across work sessions.
> This file records the current state, evidence, dependencies, and the next recommended action. It is not a substitute for source code, tests, or detailed design documents.

**Last verified:** 2026-11-27  
**Tracking baseline:** `dev` at `5e6d8ed81fc36db8f6f9e2749882420bf1f05db8` when this file was created  
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
| T1 | Record and reconcile current architecture | **IN PROGRESS** | Architecture/context documents exist; platform and test-status statements still need reconciliation |
| T2 | Complete audio-core test protection | **PARTIAL** | SoundPlayer and SoundCore tests already exist; CI validation is green on PR #4, but the test-fixture correction is not yet in `dev` |
| T3 | Audit SdContentModel | **NOT STARTED** | Responsibility and failure-mode audit remains |
| T4 | Audit UiCore | **NOT STARTED** | Facade boundary and application-flow audit remains |
| T5 | Add integration tests | **PARTIAL** | C++ SoundCore integration tests exist; QML and broader workspace/import/playlist integration remain |
| T6 | Establish CI and build validation | **IN PROGRESS** | PR #4 has green three-platform CI; it is not merged into `dev`, and Release validation remains |

---

## T1 — Record and reconcile the current architecture

**Status:** IN PROGRESS  
**Relevant documents:** `docs/ARCHITECTURE.md`, `docs/AI_CONTEXT.md`, `docs/PLATFORM_SUPPORT.md`, `README.md`, `AGENTS.md`

### Work items

- [ ] Reconcile all platform claims across README, AI_CONTEXT, PLATFORM_SUPPORT, CMake configuration, and actual CI. In particular, verify Linux minimum/distribution claims and the Windows validation wording.
- [ ] Verify and update the dependency diagram in `docs/ARCHITECTURE.md` against current source and CMake targets. Keep it a responsibility/dependency diagram, not a speculative call graph.
- [ ] Document stable public contracts at component boundaries: inputs, outputs, ownership/lifetime, error reporting, threading/async behavior, and invariants. Prioritize `SdContentModel`, `UiCore`, `SoundCore`, `SoundPlayer`, `DecodeStream`, `AudioMixer`, `AudioOutput`, and the playlist service/repository/storage boundary.
- [ ] Separate implemented automated checks, manual integration checks, platform validation, and proposed future tests. Do not describe a planned check as implemented.
- [ ] Remove or clearly label stale statements (for example, UI behavior described as planned when it is already implemented).

### Acceptance criteria

- [ ] Platform/Qt/OS statements agree across the relevant documentation and match what has actually been tested.
- [ ] The architecture diagram matches the current source layout and dependency direction.
- [ ] Each major component has a concise contract and explicit responsibility boundary.
- [ ] Test documentation distinguishes automated tests that run today from manual checks and future work.
- [ ] No known stale or contradictory architecture statements remain, or every unresolved discrepancy is explicitly recorded.

### Existing evidence / starting point

- `docs/ARCHITECTURE.md` already contains a Mermaid dependency overview and responsibility descriptions.
- `docs/AI_CONTEXT.md` contains extensive playlist, media-path, and audio invariants.
- `docs/PLATFORM_SUPPORT.md` and `README.md` currently contain platform claims that need a line-by-line consistency check. Do not treat a platform as confirmed solely because it appears in a support table.

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

- [ ] Bring the playlist test-fixture correction from PR #4 into `dev` by merging/reviewing that PR; do not copy it manually without checking whether the PR has already been merged.
- [ ] Re-run all three Qt Test targets on the updated `dev` and record the exact run/commit.
- [ ] Review remaining SoundPlayer transition/error cases against the state machine; add tests only for uncovered behavior rather than duplicating existing cases.
- [ ] Keep production QAudioSink adapter/hardware behavior explicitly classified as platform integration testing, not as covered by the fake output tests.

### Acceptance criteria

- [ ] All three Qt Test targets pass on `dev`.
- [ ] SoundPlayer state transitions and terminal paths have deterministic hardware-independent coverage.
- [ ] SoundCore-facing playback metadata/state is verified for normal completion, stop, source switch, and terminal errors.
- [ ] Any untested hardware/QML behavior is documented as a manual or separate integration check.

### Current evidence and dependency

The three test targets were run successfully on the PR #4 head `cf0f85f46c688dc533bb148002ff039d3698d7a4` (2026-11-27), on Ubuntu 22.04 / Qt 6.12.1, macOS 15 Intel / Qt 6.7.3, and Windows Server 2022 / Qt 6.12.1. Both the Push and Pull Request workflow runs succeeded:

- [Push CI run](https://github.com/DIME-TILL-DEATH/EgoApp/actions/runs/45968453268)
- [Pull Request CI run](https://github.com/DIME-TILL-DEATH/EgoApp/actions/runs/45968461705)
- [PR #4](https://github.com/DIME-TILL-DEATH/EgoApp/pull/4)

Important: PR #4 is separate from `dev`. Its test-fixture correction fixes a pre-existing assumption in `repositoryCreatesDiscoversAndRemovesNestedPlaylists()`: the shared test setup creates a default `PLAYLIST/Test` directory, while the repository test expected only the three directories it created. Until PR #4 is merged and tests are rerun on `dev`, do not report the `dev` test baseline as green.

---

## T3 — Audit SdContentModel

**Status:** NOT STARTED  
**Relevant code:** `MainApplication/sdcontentmodel.cpp/.h`, related proxy/model classes, WAV import/conversion code, and their callers.

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
**Relevant workflow:** `.github/workflows/ci.yml` (currently proposed on PR #4, not yet in `dev`).

### Work items

- [x] Define a cross-platform Qt test matrix and pin explicit Qt patch versions.
- [x] Build `playlist_tests`, `audio_tests`, and `soundcore_tests`; run them through CTest.
- [x] Preserve configure/build/test logs as workflow artifacts.
- [x] Run on Push and Pull Request.
- [x] Fix the first CI-discovered Linux runtime dependency, Windows resource-file configuration, and playlist test-fixture issue on the CI branch.
- [x] Verify green CI on Ubuntu 22.04, macOS 15 Intel, and Windows Server 2022.
- [ ] Review and merge PR #4 into `dev` (maintainer decision; do not merge without explicit authorization).
- [ ] Confirm the workflow is present and green on `dev` after merge.
- [ ] Add a Release configuration check where it can be separated from packaging/signing side effects.
- [ ] Decide which full-application build/package checks are safe and practical on hosted runners; keep minimum-OS runtime and physical audio/SD-card checks separate.

### Acceptance criteria

- [ ] CI is active on `dev` for Push and Pull Request.
- [ ] All supported test targets build and pass on the agreed matrix.
- [ ] Debug and Release validation are both covered, without accidentally running signing/deployment steps in a test-only job.
- [ ] Logs are retained and easy to retrieve after a failure.
- [ ] CI claims distinguish hosted-runner build validation from actual runtime validation on minimum OS versions.

### Platform caveat

The requested Windows 10 hosted runner is not available in the selected GitHub-hosted matrix. Windows Server 2022 validates the Windows/MSVC build and tests, but does not prove runtime compatibility with Windows 10 1809. A Windows 10 self-hosted runner or clean VM is still needed for that claim.

---

## Cross-cutting engineering rules

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

1. Review PR #4 and, once explicitly approved, merge it into `dev`; then verify that the green CI workflow and playlist fixture correction are present on `dev`.
2. Continue T1: reconcile platform/test documentation and verify the architecture diagram/contracts against current source.
3. Only after the architecture baseline is accurate, proceed to the focused SdContentModel audit (T3), then UiCore (T4).

## Tracking sources and conventions

This handoff format uses a small, repository-local checklist because the current plan has six connected workstreams and needs to remain readable without requiring a separate project-management service. The structure follows GitHub's documented practices: break work into trackable tasks, make progress/status visible, and use issues/projects for finer-grained ownership, priority, iteration, and dependency tracking when needed.

- [GitHub: Planning and tracking work](https://docs.github.com/en/issues/tracking-your-work-with-issues/learning-about-issues/planning-and-tracking-work-for-your-team-or-project)
- [GitHub: About tasklists](https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting/about-tasklists)
- [GitHub: About Projects](https://docs.github.com/en/issues/planning-and-tracking-with-projects/learning-about-projects/about-projects)
- [GitHub: Best practices for Projects](https://docs.github.com/en/issues/planning-and-tracking-with-projects/learning-about-projects/best-practices-for-projects)
