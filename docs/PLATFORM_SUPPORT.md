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