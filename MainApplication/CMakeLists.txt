cmake_minimum_required(VERSION 3.16)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MY_PRODUCT_NUMBER 1)
set(MY_PRODUCT_VERSION 1)
set(MY_BUILD_NUMBER 12)

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
    project(egogig VERSION ${MY_PRODUCT_NUMBER}.${MY_PRODUCT_VERSION}.${MY_BUILD_NUMBER} LANGUAGES CXX)

    set(PLATFORM_SPECIFIC_SOURCES
       "${PROJECT_SOURCE_DIR}/icons/EgoGIG-App.icns")

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

qt_policy(SET QTP0001 NEW)

set(QT_QML_GENERATE_QMLLS_INI ON)
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/EgoApp)
set(QML_IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY} CACHE STRING "Import paths for Qt Creator's code model" FORCE)

set_source_files_properties(AppGlobals.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt6_add_resources(RCC_FILES
    # docs.qrc
    icons.qrc
    translations.qrc
)

qt_add_qml_module(${PROJECT_NAME}
    URI EgoApp
    VERSION 1.0
    QML_FILES
        Main.qml
        PlaylistEditor.qml
        PlayerPanel.qml
        SdContentEditor.qml
        VolumeControl.qml
        AppGlobals.qml
        CheckResultWindow.qml

    SOURCES
        decodestream.h decodestream.cpp
        playlistmodel.h playlistmodel.cpp
        sdcontentmodel.h sdcontentmodel.cpp
        song.h song.cpp
        soundcore.h soundcore.cpp
        soundplayer.h soundplayer.cpp
        uicore.h uicore.cpp

        ${RCC_FILES}
        QML_FILES MainMenu.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.amtelectronics.egogig
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

find_package(Qt6 REQUIRED COMPONENTS Core Qml Gui Quick
    QuickControls2 LinguistTools Svg Multimedia)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Svg
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::Multimedia
)

set(QT_QML_GENERATE_QMLLS_INI ON)
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/)
set(QML_IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY} CACHE STRING "Import paths for Qt Creator's code model" FORCE)


# include(GNUInstallDirs)
# install(TARGETS ${PROJECT_NAME}
#     BUNDLE DESTINATION .
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
# )

if(WIN32)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(DEPLOY_PATH ${PROJECT_SOURCE_DIR}/../deploy_windows/)

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_BINARY_DIR}/MainApplication/EgoGig.exe"
                "${DEPLOY_PATH}/release/EgoGig.exe")

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${PROJECT_SOURCE_DIR}/docs"
                "${DEPLOY_PATH}/release/docs/")

        configure_file(
          ${DEPLOY_PATH}/template_script.iss
          ${DEPLOY_PATH}/installer_script.iss
          @ONLY)

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} "${DEPLOY_PATH}/release/EgoGig.exe"
                                    --qmldir "${PROJECT_SOURCE_DIR}/")

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ISCC.exe "/O${DEPLOY_PATH}/bin" "${DEPLOY_PATH}/installer_script.iss")
    endif()
endif()

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/MacOS/Info.plist"
        MACOSX_BUNDLE_BUNDLE_VERSION "${MY_PRODUCT_NUMBER}.${MY_PRODUCT_VERSION}.${MY_BUILD_NUMBER}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${MY_PRODUCT_VERSION}.${MY_BUILD_NUMBER})

    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(RELEASE_APP_PATH ${CMAKE_BINARY_DIR}/MainApplication/EgoGIG.app)

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${PROJECT_SOURCE_DIR}/docs"
                "${RELEASE_APP_PATH}/Contents/Resources/docs")

        # ditto, no cp to preserve symlinks
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${MACDEPLOYQT_EXECUTABLE} "${RELEASE_APP_PATH}"
                                    "-qmldir=${PROJECT_SOURCE_DIR}/"
                                    "-appstore-compliant")

        add_custom_command(TARGET ${PROJECT_NAME}  POST_BUILD
            COMMAND ditto #cp "-a"
            "${CMAKE_BINARY_DIR}/WavConverterShell/wavconverter.app"
            "${RELEASE_APP_PATH}/Contents/MacOS/wavconverter.app")

        set(DEPLOY_PATH /Applications/)
        add_custom_command(TARGET ${PROJECT_NAME}  POST_BUILD
            COMMAND ditto #cp "-a"
            "${RELEASE_APP_PATH}"
            "${DEPLOY_PATH}/EgoGIG.app")

        set(APPLE_CERTIFICATE_APP_STORE "Apple Distribution: AMT corporation S.R.L. (4PGXCAZDT5)")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND codesign "-f" "--deep" "-s" "${APPLE_CERTIFICATE_APP_STORE}"
                "--entitlements" "${PROJECT_SOURCE_DIR}/../deploy_mac/entitlements.xml"
                "${DEPLOY_PATH}/EgoGIG.app"
                # "${PROJECT_SOURCE_DIR}/../deploy_mac/AppStore/EgoGig.app"
                VERBATIM)

        # pkgbuild --identifier "com.amtelectronics.egogig.pkg" --version $VERSION --component "${{ env.AU_PATH }}" --install-location "/Library/Audio/Plug-Ins/Components" "packaging/${{ env.PRODUCT_NAME }}.au.pkg"


        set(APPLE_CERTIFICATE_APP_STORE_PKG "3rd Party Mac Developer Installer: AMT corporation S.R.L. (4PGXCAZDT5)")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND productbuild "--sign"
                "${APPLE_CERTIFICATE_APP_STORE_PKG}"
                "--component" "${DEPLOY_PATH}/EgoGig.app"
                "${PROJECT_SOURCE_DIR}/../deploy_mac/AppStore/EgoGig.pkg"
                VERBATIM)


        set(APPLE_CERTIFICATE_STANDALONE "Developer ID Application: AMT corporation S.R.L. (4PGXCAZDT5)")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND codesign "-f" "--deep" "-s" "${APPLE_CERTIFICATE_STANDALONE}"
                "--options=runtime"
                "--timestamp"
                "${DEPLOY_PATH}/EgoGIG.app"
                VERBATIM)

        set(APPLE_CERTIFICATE_STANDALONE_PKG "Developer ID Installer: AMT corporation S.R.L. (4PGXCAZDT5)")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND productbuild "--sign"
            "${APPLE_CERTIFICATE_STANDALONE_PKG}"
            "--component" "${DEPLOY_PATH}/EgoGig.app"
            "${PROJECT_SOURCE_DIR}/../deploy_mac/Standalone/EgoGig.pkg"
            VERBATIM)

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND rm -r
                "${DEPLOY_PATH}/EgoGig.app")

    endif()
endif()

if(LINUX)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(DEPLOY_PATH ${PROJECT_SOURCE_DIR}/../deploy_linux/)

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_BINARY_DIR}/MainApplication/EgoGig"
            "${DEPLOY_PATH}app/bin/EgoGIG")

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/docs"
            "${DEPLOY_PATH}app/docs/")

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${DEPLOY_PATH}/libs"
            "${DEPLOY_PATH}/app/lib")


        # using cqtdeployer, installed from snap
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND cqtdeployer -bin "${DEPLOY_PATH}/app/bin/EgoGIG"
                                -targetDir "${DEPLOY_PATH}/app/"
                                -libDir "${PROJECT_SOURCE_DIR}/../sox_lib/lib.linux"
                                -extralibs "libavformat"
                                -qmlDir "${PROJECT_SOURCE_DIR}/"
                                -qmake "~/Qt/6.8.1/gcc_64/bin/qmake")

        # make tarball archive
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND tar -czvf "${DEPLOY_PATH}/EgoGIG.tar.gz" "${DEPLOY_PATH}/app/")

        # make .deb package
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${DEPLOY_PATH}/app/"
            "${DEPLOY_PATH}/debian_deploy/EgoGIG/usr/local/EgoGIG/")

        configure_file(
            ${DEPLOY_PATH}/DEBIAN/control
            ${DEPLOY_PATH}/debian_deploy/EgoGIG/DEBIAN/control
            @ONLY)

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${DEPLOY_PATH}/usr/"
            "${DEPLOY_PATH}/debian_deploy/EgoGIG/usr/")

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND dpkg-deb --build "${DEPLOY_PATH}/debian_deploy/EgoGIG/")
    endif()
endif()
