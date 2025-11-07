cmake_minimum_required (VERSION 3.21.1)

project(EgoGig)

add_subdirectory (WavConverterLib)
if(NOT ANDROID AND NOT IOS)
add_subdirectory (WavConverterShell)
endif()
add_subdirectory (MainApplication)


