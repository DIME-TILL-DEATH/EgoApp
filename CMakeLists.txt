cmake_minimum_required (VERSION 3.21.1)

# ---------------------------------------------------------------------------
# macOS: minimum supported OS (single source of truth).
# CMAKE_OSX_DEPLOYMENT_TARGET MUST be set before the first project() call,
# otherwise the Mach-O "minos" of every binary defaults to the macOS version
# of the build machine and the app will not start on macOS 11.
# ---------------------------------------------------------------------------
set(EGOGIG_MIN_MACOS "11.0")
if(APPLE AND NOT IOS)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "${EGOGIG_MIN_MACOS}" CACHE STRING "Minimum macOS version the app is deployed to")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
endif()

project(EgoGIG)

include(CTest)

option(EGOGIG_VERIFY_MIN_MACOS "macOS Release: fail the build if any Mach-O/Info.plist requires a newer OS than EGOGIG_MIN_MACOS" ON)
option(EGOGIG_PRUNE_BUNDLE "macOS Release: remove Qt modules the app does not use from the .app after macdeployqt" ON)
option(EGOGIG_ENABLE_RELEASE_PACKAGING "Enable platform deployment, signing, and installer post-build steps" ON)

if(APPLE AND NOT IOS)
    if(CMAKE_OSX_DEPLOYMENT_TARGET VERSION_GREATER EGOGIG_MIN_MACOS)
        message(FATAL_ERROR
            "CMAKE_OSX_DEPLOYMENT_TARGET is '${CMAKE_OSX_DEPLOYMENT_TARGET}', "
            "but the project must support macOS ${EGOGIG_MIN_MACOS}. "
            "A toolchain file or a stale CMakeCache.txt overrides it: "
            "delete the build directory and configure again.")
    endif()

    # Qt 6.8+ officially supports macOS 12+ only; Qt docs warn that lowering
    # the deployment target below Qt's own default is likely to crash.
    find_package(Qt6 REQUIRED COMPONENTS Core)
    if(Qt6_VERSION VERSION_GREATER_EQUAL 6.8 AND NOT EGOGIG_SKIP_QT_OS_CHECK)
        message(FATAL_ERROR
            "Qt ${Qt6_VERSION} requires macOS 12 or newer, but macOS "
            "${EGOGIG_MIN_MACOS} is required. Use Qt 6.7.x for macOS builds "
            "(pass -DEGOGIG_SKIP_QT_OS_CHECK=ON to override at your own risk).")
    endif()
endif()

add_subdirectory (WavConverterLib)

# The standalone converter is not needed when configuring only the automated
# MainApplication test suites. Keep it enabled by default for normal builds.
option(EGOGIG_BUILD_CONVERTER "Build the standalone WavConverter application" ON)
if(NOT ANDROID AND NOT IOS AND EGOGIG_BUILD_CONVERTER)
    add_subdirectory (WavConverterShell)
endif()

add_subdirectory (MainApplication)

