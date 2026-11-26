cmake_minimum_required(VERSION 3.16)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MY_PRODUCT_NUMBER 1)
set(MY_PRODUCT_VERSION 1)
set(MY_BUILD_NUMBER 14)

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
        MainMenu.qml

    SOURCES
        decodestream.h decodestream.cpp
        playlistmodel.h playlistmodel.cpp
        sdcontentmodel.h sdcontentmodel.cpp
        song.h song.cpp
        soundcore.h soundcore.cpp
        soundplayer.h soundplayer.cpp
        uicore.h uicore.cpp

        ${RCC_FILES}
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


if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

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
        MACOSX_BUNDLE_GUI_IDENTIFIER com.amtelectronics.egogig
        MACOSX_BUNDLE_BUNDLE_VERSION "${MY_PRODUCT_NUMBER}.${MY_PRODUCT_VERSION}.${MY_BUILD_NUMBER}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${MY_PRODUCT_VERSION}.${MY_BUILD_NUMBER}
        MACOSX_BUNDLE TRUE)


    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(RELEASE_APP_PATH ${CMAKE_BINARY_DIR}/MainApplication/EgoGIG.app)

        add_dependencies(${PROJECT_NAME} wavconverter)

        # Вся промежуточная упаковка — только внутри папки сборки.
        set(MAC_STAGING ${CMAKE_BINARY_DIR}/mac_staging)
        cmake_path(IS_PREFIX CMAKE_BINARY_DIR "${MAC_STAGING}" NORMALIZE MAC_STAGING_INSIDE_BUILD)
        if(NOT MAC_STAGING_INSIDE_BUILD)
            message(FATAL_ERROR "MAC_STAGING must be inside the build dir: ${MAC_STAGING}")
        endif()

        set(MAC_DEPLOY_DIR ${PROJECT_SOURCE_DIR}/../deploy_mac)

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${PROJECT_SOURCE_DIR}/docs"
                "${RELEASE_APP_PATH}/Contents/Resources/docs")

        # WavConverter is a second executable next to the main one (not a nested
        # .app), so both share Contents/Frameworks, PlugIns and Resources/qml.
        set(BUNDLED_CONVERTER "${RELEASE_APP_PATH}/Contents/MacOS/wavconverter")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                # remove the nested wavconverter.app left by older builds
                COMMAND ${CMAKE_COMMAND} -E rm -rf "${RELEASE_APP_PATH}/Contents/MacOS/wavconverter.app"
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:wavconverter> "${BUNDLED_CONVERTER}"
                VERBATIM)

        # -executable: macdeployqt also deploys what the converter needs and
        # points it to the shared Frameworks. The converter's QML (e.g.
        # Qt.labs.platform) is scanned too, otherwise it is missing at runtime.
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${MACDEPLOYQT_EXECUTABLE} "${RELEASE_APP_PATH}"
                                    "-qmldir=${PROJECT_SOURCE_DIR}/"
                                    "-qmldir=${PROJECT_SOURCE_DIR}/../WavConverterShell/"
                                    "-executable=${BUNDLED_CONVERTER}"
                                    "-libpath=$<TARGET_FILE_DIR:converter-lib>"
                                    "-appstore-compliant"
                VERBATIM)

        # Drop Qt modules/styles/plugins that macdeployqt copied but the app does
        # not use (Qt3D, VirtualKeyboard, unused Controls styles, ...).
        if(EGOGIG_PRUNE_BUNDLE)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND bash "${MAC_DEPLOY_DIR}/prune_bundle.sh" "${RELEASE_APP_PATH}"
                VERBATIM)
        endif()

        # Fail early (before signing/packaging) if anything in the bundle needs
        # a newer macOS than EGOGIG_MIN_MACOS (see top-level CMakeLists.txt).
        if(EGOGIG_VERIFY_MIN_MACOS)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND bash "${MAC_DEPLOY_DIR}/verify_min_macos.sh"
                        "${RELEASE_APP_PATH}" "${EGOGIG_MIN_MACOS}"
                VERBATIM)
        endif()

        #################### App store ######################
        set(MAS_DIR ${MAC_STAGING}/appstore)
        set(MAS_APP ${MAS_DIR}/EgoGIG.app)

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${MAS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${MAS_DIR}"
            COMMAND ditto "${RELEASE_APP_PATH}" "${MAS_APP}"
            VERBATIM)

        set(APPLE_CERTIFICATE_APP_STORE "Apple Distribution: AMT corporation S.R.L. (4PGXCAZDT5)")
        # Signing order matters: (1) everything, (2) the helper executable with
        # sandbox-inheritance entitlements (the Mac App Store requires every
        # executable to be sandboxed; a child process must inherit the sandbox
        # of EgoGIG, otherwise it crashes at start), (3) re-seal the app bundle,
        # because step 2 changed a file inside it.
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND codesign "-f" "--deep" "-s" "${APPLE_CERTIFICATE_APP_STORE}"
                    "--entitlements" "${MAC_DEPLOY_DIR}/entitlements.xml"
                    "${MAS_APP}"
            COMMAND codesign "-f" "-s" "${APPLE_CERTIFICATE_APP_STORE}"
                    "-i" "com.amtelectronics.egogig.wavconverter"
                    "--entitlements" "${MAC_DEPLOY_DIR}/entitlements_inherit.xml"
                    "${MAS_APP}/Contents/MacOS/wavconverter"
            COMMAND codesign "-f" "-s" "${APPLE_CERTIFICATE_APP_STORE}"
                    "--entitlements" "${MAC_DEPLOY_DIR}/entitlements.xml"
                    "${MAS_APP}"
            COMMAND codesign "--verify" "--deep" "--strict" "--verbose=2" "${MAS_APP}"
            COMMAND bash "${MAC_DEPLOY_DIR}/check_signing.sh" "${MAS_APP}" appstore
            VERBATIM)

        set(APPLE_CERTIFICATE_APP_STORE_PKG "3rd Party Mac Developer Installer: AMT corporation S.R.L. (4PGXCAZDT5)")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${MAC_DEPLOY_DIR}/AppStore"
            COMMAND productbuild "--sign" "${APPLE_CERTIFICATE_APP_STORE_PKG}"
                    "--component" "${MAS_APP}" "/Applications"
                    "${MAC_DEPLOY_DIR}/AppStore/EgoGig.pkg"
            VERBATIM)

        ##################### STANDALONE ###################################
        set(STANDALONE_ROOT ${MAC_STAGING}/standalone)
        set(STANDALONE_APP ${STANDALONE_ROOT}/Payload/EgoGIG.app)

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${STANDALONE_ROOT}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${STANDALONE_ROOT}/Payload"
            COMMAND ditto "${RELEASE_APP_PATH}" "${STANDALONE_APP}"
            VERBATIM)

        set(APPLE_CERTIFICATE_STANDALONE "Developer ID Application: AMT corporation S.R.L. (4PGXCAZDT5)")

        # add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        #     COMMAND codesign "-f" "--deep" "-s" "${APPLE_CERTIFICATE_STANDALONE}"
        #             "--options=runtime" "--timestamp"
        #             "${STANDALONE_APP}"
        #     COMMAND codesign "-f" "-s" "${APPLE_CERTIFICATE_STANDALONE}"
        #             "-i" "com.amtelectronics.egogig.wavconverter"
        #             "--options=runtime" "--timestamp"
        #             "${STANDALONE_APP}/Contents/MacOS/wavconverter"
        #     COMMAND codesign "-f" "-s" "${APPLE_CERTIFICATE_STANDALONE}"
        #             "--options=runtime" "--timestamp"
        #             "${STANDALONE_APP}"
        #     COMMAND codesign "--verify" "--deep" "--strict" "--verbose=2" "${STANDALONE_APP}"
        #     COMMAND bash "${MAC_DEPLOY_DIR}/check_signing.sh" "${STANDALONE_APP}" standalone
        #     VERBATIM)

        # add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        #     COMMAND pkgbuild
        #             "--identifier" "com.amtelectronics.egogig"
        #             "--root" "${STANDALONE_ROOT}/Payload"
        #             "--component-plist" "${MAC_DEPLOY_DIR}/Standalone/pkg.plist"
        #             "--install-location" "/Applications"
        #             "${STANDALONE_ROOT}/EgoGig-unsigned.pkg"
        #     VERBATIM)

        # set(APPLE_CERTIFICATE_STANDALONE_PKG "Developer ID Installer: AMT corporation S.R.L. (4PGXCAZDT5)")
        # add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        #     COMMAND productbuild "--sign" "${APPLE_CERTIFICATE_STANDALONE_PKG}"
        #             "--package" "${STANDALONE_ROOT}/EgoGig-unsigned.pkg"
        #             "${MAC_DEPLOY_DIR}/Standalone/EgoGig.pkg"
        #     VERBATIM)

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
