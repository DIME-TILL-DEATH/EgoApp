cmake_minimum_required(VERSION 3.16)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MY_PRODUCT_NUMBER 1)
set(MY_PRODUCT_VERSION 1)
set(MY_BUILD_NUMBER 0)

if(WIN32)
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/version.rc.in
      ${CMAKE_CURRENT_SOURCE_DIR}/version.rc
      @ONLY)

    project(EgoGig VERSION ${MY_PRODUCT_NUMBER}.${MY_PRODUCT_VERSION}.${MY_BUILD_NUMBER}.1 LANGUAGES CXX)

    set(PLATFORM_SPECIFIC_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/icons/icon.rc
        ${CMAKE_CURRENT_SOURCE_DIR}/version.rc
    )

elseif(APPLE) # MacOS
    project(EgoGig VERSION ${MY_PRODUCT_NUMBER}.${MY_PRODUCT_VERSION}.${MY_BUILD_NUMBER} LANGUAGES CXX)

    set(PLATFORM_SPECIFIC_SOURCES
       "${PROJECT_SOURCE_DIR}/icons/pangaea.icns")

   set_source_files_properties(${PLATFORM_SPECIFIC_SOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
elseif(LINUX)
    project(EgoGig VERSION ${MY_PRODUCT_NUMBER}.${MY_PRODUCT_VERSION}.${MY_BUILD_NUMBER} LANGUAGES CXX)

    add_definitions(-DVERSION_STRING="${MY_PRODUCT_NUMBER}.${MY_PRODUCT_VERSION}.${MY_BUILD_NUMBER}")
endif()

# qt6_add_resources(RCC_FILES
#     "images.qrc"
#     "translations.qrc"
# )

find_package(Qt6 REQUIRED COMPONENTS Quick)



qt_add_executable(${PROJECT_NAME}
    main.cpp

    ${PLATFORM_SPECIFIC_SOURCES}
    ${RCC_FILES}
)

# target_include_directories(${PROJECT_NAME} PUBLIC

target_link_libraries(EgoGig PRIVATE Qt6::Core)

# )

qt_policy(SET QTP0001 NEW)
qt_add_qml_module(${PROJECT_NAME}
    URI EgoApp
    VERSION 1.0
    QML_FILES
        Main.qml
        SOURCES uicore.h uicore.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(${PROJECT_NAME} PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appEgoApp
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

find_package(Qt6 REQUIRED COMPONENTS Qml Gui Quick LinguistTools Svg)
find_package(Qt6 REQUIRED COMPONENTS Core)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Svg
    Qt6::Qml
)

set(QT_QML_GENERATE_QMLLS_INI ON)
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/PangaeaFrontend/qml)
set(QML_IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY} CACHE STRING "Import paths for Qt Creator's code model" FORCE)


include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
