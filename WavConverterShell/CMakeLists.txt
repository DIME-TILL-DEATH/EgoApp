cmake_minimum_required(VERSION 3.21.1)

find_package(Qt6 REQUIRED COMPONENTS Core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(WIN32)
    project(WavConverter VERSION 1.0.0 LANGUAGES CXX)
    add_compile_definitions(WAVCONVERTERLIB_LIBRARY)

    set(PLATFORM_SPECIFIC_SOURCES
        resources.rc
    )

elseif(APPLE)
    project(wavconverter VERSION 1.0.0 LANGUAGES CXX)

    set(PLATFORM_SPECIFIC_SOURCES
       "${PROJECT_SOURCE_DIR}/icons/EgoGig-Converter.icns")

    set_source_files_properties(${PLATFORM_SPECIFIC_SOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")


endif()

qt6_add_resources(RCC_FILES
    "qml.qrc"
)

qt_add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
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

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.amtelectronics.wavconverter
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(WIN32)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
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
    # set_target_properties(${PROJECT_NAME} PROPERTIES
    #     MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/MacOS/Info.plist")

    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(RELEASE_APP_PATH ${CMAKE_BINARY_DIR}/WavConverterShell/WavConverter.app)

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${MACDEPLOYQT_EXECUTABLE} "${RELEASE_APP_PATH}"
                    "-qmldir=${PROJECT_SOURCE_DIR}/"
                    "-libpath=${CMAKE_BINARY_DIR}/../WavConverterLib/"
                    "-appstore-compliant")
    endif()
endif()

if(LINUX)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_BINARY_DIR}/WavConverterShell/WavConverter"
                "${PROJECT_SOURCE_DIR}/../deploy_linux/app/bin/WavConverter")
    endif()
endif()
