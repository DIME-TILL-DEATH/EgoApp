cmake_minimum_required(VERSION 3.21.1)

find_package(Qt6 REQUIRED COMPONENTS Core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(WavConverter VERSION 1.0.0 LANGUAGES CXX)

if(WIN32)
    add_compile_definitions(WAVCONVERTERLIB_LIBRARY)

    set(PLATFORM_SPECIFIC_SOURCES
        resources.rc
    )
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

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_BINARY_DIR}/WavConverterShell/IrConverter.exe"
            "${PROJECT_SOURCE_DIR}/../deploy_windows/release/IrConverter.exe")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} "${PROJECT_SOURCE_DIR}/../deploy_windows/release/IrConverter.exe"
                                --qmldir "${PROJECT_SOURCE_DIR}/")
endif()

if(LINUX)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_BINARY_DIR}/WavConverterShell/IrConverter"
            "${PROJECT_SOURCE_DIR}/../deploy_linux/app/bin/IrConverter")
endif()
