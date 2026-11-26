# Platform Support

*Target versions were specified by the project owner. Support is not confirmed until builds and runtime checks pass on the minimum OS.*

| Platform | Qt | Minimum target OS | Status |
|---|---:|---|---|
| macOS | 6.7 | macOS 11 | Validation required |
| Windows | 6.11/6.12 | Windows 10 1809 | Confirmed |
| Linux | 6.11/6.12 | Ubuntu 22.04 | Confirmed |

## General validation
Check Qt/OS APIs, compiler and C++ runtime, system libraries/ABI, Qt plugins, audio backend/codecs, signing and packaging, removable-media behavior, and launch on the minimum OS. A build on a current system does not establish compatibility with an older one.

## macOS
Qt 6.7; minimum macOS 11. Validate deployment target, binary architectures, Qt plugins, signing, and launch on a clean macOS 11 installation. Do not silently raise the deployment target.

Where the minimum OS is defined (keep these in sync):
- `CMakeLists.txt` (top level): `EGOGIG_MIN_MACOS` -> `CMAKE_OSX_DEPLOYMENT_TARGET`, set before `project()`. This sets the Mach-O `minos` of all binaries built from this tree. Universal build (`x86_64;arm64`) by default.
- `MainApplication/MacOS/Info.plist`: `LSMinimumSystemVersion` (Finder/LaunchServices refuse to open the app below it). The nested converter's `WavConverterShell/MacOS/Info.plist` is currently unused but kept consistent.
- `deploy_mac/verify_min_macos.sh`: runs after `macdeployqt` in Release builds and fails the build if the `.app`, a nested app, or any bundled Qt framework/plugin/dylib requires a newer macOS. Disable with `-DEGOGIG_VERIFY_MIN_MACOS=OFF`.

Bundle contents: `deploy_mac/check_bundle.sh` runs after `macdeployqt` and only reads the bundle: it fails if any binary links a Qt framework that is not in `Contents/Frameworks`, or if a QML module needed by EgoGIG/WavConverter is missing from `Resources/qml`, and prints the size of the largest items. Disable with `-DEGOGIG_CHECK_BUNDLE=OFF`. Frameworks are deliberately NOT removed by hand: `macdeployqt` places QML plugins in both `Resources/qml` and `PlugIns/quick`, so deleting "unused" modules is fragile and saved only ~17 MB of ~167 MB.

WavConverter on macOS is a second executable, `EgoGIG.app/Contents/MacOS/wavconverter`, not a nested `.app`:
- `WavConverterShell` builds a plain executable; `MainApplication` copies it into the bundle and runs `macdeployqt -executable=...` with both QML directories, so the converter uses the single shared `Contents/Frameworks`, `PlugIns`, `Resources/qml`. `UiCore::runWavConvertor()` starts it with `QProcess`.
- Consequence: the converter has no `.app` identity of its own (Dock icon/name of EgoGIG, no `EgoGig-Converter.icns`; `WavConverterShell/MacOS/Info.plist` is unused).
- Signing (`MainApplication/CMakeLists.txt`): `codesign --deep` for everything, then the helper separately, then the app again to re-seal. App Store: helper gets `deploy_mac/entitlements_inherit.xml` (`app-sandbox` + `inherit`, nothing else is allowed with `inherit`). Standalone: hardened runtime + timestamp on the helper too. `deploy_mac/check_signing.sh` verifies this after signing.
- App Store caveat: a child with `inherit` runs in the parent's sandbox and does not get access to files the user picks in its OWN open/save panels (Apple DTS: inheritance is meant for helper tools without UI). Test the sandboxed build: open files and choose the output folder in the converter. If that fails, the converter must become part of the EgoGIG process (or a separate sandboxed `.app` started through Launch Services).
- `deploy_mac/Standalone/pkg.plist` only lists the top-level `EgoGIG.app` (it used to list ~130 bundles that no longer exist after pruning/de-duplication).

Qt 6.7 supports macOS 11-14; Qt 6.8+ supports macOS 12+ only, so macOS 11 builds must use Qt 6.7.x (configure fails otherwise).

## Windows
Qt 6.11 or 6.12; minimum Windows 10 version 1809. Validate toolchain, MSVC runtime, Qt DLLs/plugins, audio backend, and installer. Test on clean Windows 10 1809. APIs newer than 1809 need a tested fallback.

## Linux
Qt 6.11 or 6.12; minimum OS Ubuntu 24.04 package format-.dpkg. Validate glibc, libstdc++, Qt, and audio dependencies. Do not claim support for all distributions without an ABI contract.

## CI and release
Recommended: pinned-Qt builds for all platforms, model and file-operation tests, packaging/plugin checks, smoke/runtime tests on minimum OS or clean VM, and manual SD-card/audio-hardware checks before release.

## Checklist
- [ ] Minimum Qt/OS versions checked.
- [ ] New APIs have a fallback.
- [ ] CI and build instructions updated.
- [ ] Runtime dependencies and packaging checked.
- [ ] Minimum-OS launch tested.
- [ ] Matrix and release notes updated.