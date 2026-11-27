cmake_minimum_required(VERSION 3.21.1)
project(converter-lib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

if(IOS)
    qt_add_library(converter-lib STATIC
        resampler.cpp resampler.h
        wavutils.cpp wavutils.h
    )
else()
    qt_add_library(converter-lib SHARED
        resampler.cpp resampler.h
        wavutils.cpp wavutils.h
    )
endif()

if(ANDROID)
    target_link_libraries(converter-lib PUBLIC
        Qt6::Core

        ${CMAKE_CURRENT_SOURCE_DIR}/../sox_lib/lib.android/${CMAKE_ANDROID_ARCH_ABI}/libsox.so
    )
endif()

if(LINUX)
    target_link_libraries(converter-lib PUBLIC
        Qt6::Core

        ${CMAKE_CURRENT_SOURCE_DIR}/../sox_lib/lib.linux/libsox.so        
    )

    if(EGOGIG_ENABLE_RELEASE_PACKAGING AND CMAKE_BUILD_TYPE STREQUAL "Release")
        set(DEPLOY_PATH ${PROJECT_SOURCE_DIR}/../deploy_linux/)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_BINARY_DIR}/WavConverterLib/libconverter-lib.so"
                "${DEPLOY_PATH}/app/lib/libconverter-lib.so")

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/../sox_lib/lib.linux/"
                "${DEPLOY_PATH}/app/lib/")
    endif()
endif()

if(WIN32)
    add_compile_definitions(WAVCONVERTERLIB_LIBRARY)

    set_target_properties(converter-lib PROPERTIES OUTPUT_NAME WavConverterLib)

    target_link_libraries(converter-lib PUBLIC
        Qt6::Core

        ${CMAKE_CURRENT_SOURCE_DIR}/../sox_lib/lib.msvc2019/libsox.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../sox_lib/lib.msvc2019/liblpc10.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../sox_lib/lib.msvc2019/libgsm.lib
    )

    if(EGOGIG_ENABLE_RELEASE_PACKAGING AND CMAKE_BUILD_TYPE STREQUAL "Release")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_BINARY_DIR}/WavConverterLib/WavConverterLib.dll"
                "${PROJECT_SOURCE_DIR}/../deploy_windows/release/WavConverterLib.dll")
    endif()
endif()


if(APPLE)
    # libsox.a собрана с обработчиком CoreAudio, ему нужны системные фреймворки
    find_library(COREAUDIO_FRAMEWORK CoreAudio REQUIRED)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)

    target_link_libraries(converter-lib PUBLIC
        Qt6::Core

        ${CMAKE_CURRENT_SOURCE_DIR}/../sox_lib/lib.mac/libsox.a
        ${COREAUDIO_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
        ${COREFOUNDATION_FRAMEWORK}
    )
endif()


target_include_directories(converter-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../sox_lib/include/
)
