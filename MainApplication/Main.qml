import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

// import CppObjects

ApplicationWindow {
    id: _mainWindow

    width: 800
    height: 400

    minimumWidth: 600
    maximumWidth: 2000
    minimumHeight: 400
    maximumHeight: 1000

    visible: true
    title: qsTr("EgoGig")

    Settings{
        property alias width: _mainWindow.width
        property alias height: _mainWindow.height

        property alias workspacePath: _folderDialog.currentFolder
    }


    header: MenuBar{
        id: mainMenu

        Menu{
            title: qsTr("File")

            MenuSeparator{}
            MenuItem{
                text: qsTr("Quit")
                onTriggered: {Qt.quit()}
            }
        }

        Menu{
            title: qsTr("Interface")

            MenuItem{
                text: qsTr("Show hotkeys")

                onTriggered: _playlistEditor.hotKeysDialog.open()
            }
        }

        Menu{
            title: qsTr("Tools")

            MenuItem{
                text: qsTr("Legacy WAV convertor")

                onTriggered: UiCore.runWavConvertor();
            }
        }

        Menu{
            title: qsTr("Help")

            MenuItem{
                text: qsTr("AMT web site")

                onTriggered: Qt.openUrlExternally("http://www.amtelectronics.ru/");
            }

            MenuItem{
                text: qsTr("Pangaea web page")

                onTriggered: Qt.openUrlExternally("https://media.amtelectronics.ru/amt-egogig-eg-4/")
            }

            MenuItem{
                id: menuDeviceManual

                property string strManualBaseName: "EgoGIG_manual"

                text: qsTr("Device manual")

                onTriggered: {
                    UiCore.openManualExternally(strManualBaseName)
                }
            }
            MenuSeparator{}
            MenuItem{
                text: qsTr("About...")
                onTriggered: aboutDialog.open()
            }
        }

    }

    GridLayout{
        anchors.fill: parent

        columns: 5

        Label{
            text: qsTr("Workspace:")
            Layout.columnSpan: 2
            horizontalAlignment: Label.AlignHCenter
        }

        Button{
            Layout.columnSpan: 3
            Layout.fillWidth: true

            text: UiCore.workspacePath

            onClicked: _folderDialog.open()

            FolderDialog{
                id: _folderDialog

                onAccepted: UiCore.setWorkspace(_folderDialog.selectedFolder)
            }
        }

        VolumeControl{
            Layout.preferredWidth: parent.width / 9 / 2
            Layout.fillHeight: true

            value: SoundCore.track1Volume
            muted: SoundCore.track1Muted
            leftEnabled: SoundCore.track1LEnabled
            rightEnabled: SoundCore.track1REnabled

            label: "T1"

            onSliderValueChanged: (newValue) => {
                                    SoundCore.track1Volume = newValue
                                }

            onTrackMuteChanged: {
                SoundCore.track1Muted = !SoundCore.track1Muted
            }

            onLeftMuteChanged: (newValue) => {
                                   SoundCore.track1LEnabled = newValue
                               }

            onRightMuteChanged: (newValue) => {
                                   SoundCore.track1REnabled = newValue
                               }
        }

        VolumeControl{
            Layout.preferredWidth: parent.width / 9 / 2
            Layout.fillHeight: true

            value: SoundCore.track2Volume
            muted: SoundCore.track2Muted
            leftEnabled: SoundCore.track2LEnabled
            rightEnabled: SoundCore.track2REnabled

            label: "T2"

            onSliderValueChanged: (newValue) => {
                                    SoundCore.track2Volume = newValue
                                }

            onTrackMuteChanged: {
                SoundCore.track2Muted = !SoundCore.track2Muted
            }

            onLeftMuteChanged: (newValue) => {
                                   SoundCore.track2LEnabled = newValue
                               }

            onRightMuteChanged: (newValue) => {
                                   SoundCore.track2REnabled = newValue
                               }
        }

        PlaylistEditor{
            id: _playlistEditor

            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        SdContentEditor{
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        VolumeControl{
            Layout.preferredWidth: parent.width / 9 / 2
            Layout.fillHeight: true

            value: SoundCore.sdTrackVolume
            muted: SoundCore.sdTrackMuted
            leftEnabled: SoundCore.sdTrackLEnabled
            rightEnabled: SoundCore.sdTrackREnabled

            label: "WAV"

            onSliderValueChanged: (newValue) => {
                                    SoundCore.sdTrackVolume = newValue
                                }

            onTrackMuteChanged: {
                SoundCore.sdTrackMuted = !SoundCore.sdTrackMuted
            }

            onLeftMuteChanged: (newValue) => {
                                   SoundCore.sdTrackLEnabled = newValue
                               }

            onRightMuteChanged: (newValue) => {
                                   SoundCore.sdTrackREnabled = newValue
                               }
        }
    }

    Popup{
        id: _wait

        width: parent.width
        height: parent.height

        modal: true

        background: Rectangle{
            opacity: 0.6

            color: palette.alternateBase
        }

        Column{
            anchors.fill: parent
            BusyIndicator{
                running: true

                anchors.horizontalCenter: parent.horizontalCenter

                width: parent.width /4
                height: width
            }

            Label{
                id: _textString
                width: parent.width

                property int filesLeft: 0

                font.bold: true

                horizontalAlignment: Label.AlignHCenter

                text: qsTr("Converting and uploading. Files left: ") + filesLeft
            }

            ProgressBar{
                id: _progressBar

                anchors.horizontalCenter: parent.horizontalCenter

                width: parent.width / 3
            }
        }

        anchors.centerIn: parent
        closePolicy: Popup.NoAutoClose

        Connections{
            target: UiCore.sdContentModel

            function onDecodingStarted(filesLeft){
                _textString.filesLeft = filesLeft;
                _progressBar.value = 0;
                _wait.open()
            }

            function onDecodingUpdated(processed, fileDuration){
                _progressBar.to = fileDuration;
                _progressBar.value += processed;
            }

            function onDecodingFinished(){
                _wait.close()
            }
        }
    }

    MessageDialog{
        id: aboutDialog

        title: qsTr("About...")
        text: qsTr("AMT EgoGIG")
        informativeText: qsTr("Desktop application") + "\n" +
              qsTr("Version: ") + Qt.application.version + "\n"
              + qsTr("(c) 2025")

    }

    MessageDialog{
        id: _errorDialog

        title: qsTr("Error!")
    }

    Connections{
        target: UiCore

        function onErrorOccured(description){
            console.log("qml error dialog")

            _errorDialog.informativeText = description
            _errorDialog.open();
        }
    }
}
