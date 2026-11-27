cmake_minimum_required(VERSION 3.21.1)

find_package(Qt6 REQUIRED COMPONENTS Core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(WIN32)
    project(WavConverter VERSION 1.0.0 LANGUAGES CXX)
    add_compile_definitions(WAVCONVERTERLIB_LIBRARY)

    # Generate the converter icon resource in the build tree so a clean
    # checkout can configure the complete Windows application.
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/resources.rc"
        @ONLY
    )

    set(PLATFORM_SPECIFIC_SOURCES
        "${CMAKE_CURRENT_BINARY_DIR}/resources.rc"
    )

elseif(APPLE)
    # On macOS the converter is NOT a separate .app: it is a plain executable
    # that MainApplication copies into EgoGIG.app/Contents/MacOS and that shares
    # the main app's Frameworks/PlugIns/qml (one copy of Qt instead of two).
    project(wavconverter VERSION 1.0.0 LANGUAGES CXX)

endif()

qt6_add_resources(RCC_FILES
    "qml.qrc"
)

qt_add_executable(${PROJECT_NAME} WIN32
    core.cpp core.h
    main.cpp

    ${RCC_FILES}
    ${PLATFORM_SPECIFIC_SOURCES}
)



target_include_directories(${PROJECT_NAME} PUBLIC
    ../WavConverterLib/
)

find_package(Qt6 REQUIRED COMPONENTS Qml Quick QuickControls2 Concurrent)
target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml

    converter-lib
)



if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    if(EGOGIG_ENABLE_RELEASE_PACKAGING AND CMAKE_BUILD_TYPE STREQUAL "Release")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_BINARY_DIR}/WavConverterShell/WavConverter.exe"
                "${PROJECT_SOURCE_DIR}/../deploy_windows/release/WavConverter.exe")

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} "${PROJECT_SOURCE_DIR}/../deploy_windows/release/WavConverter.exe"
                                    --qmldir "${PROJECT_SOURCE_DIR}/")
    endif()
endif()

if(APPLE)
    # Plain (non-bundle) executable: build output is
    #   ${CMAKE_BINARY_DIR}/WavConverterShell/wavconverter
    # Deployment (macdeployqt -executable=..., signing) is done by MainApplication.
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE FALSE)
endif()

if(LINUX)
    if(EGOGIG_ENABLE_RELEASE_PACKAGING AND CMAKE_BUILD_TYPE STREQUAL "Release")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_BINARY_DIR}/WavConverterShell/WavConverter"
                "${PROJECT_SOURCE_DIR}/../deploy_linux/app/bin/WavConverter")
    endif()
endif()
