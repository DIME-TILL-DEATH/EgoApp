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
        }

        Menu{
            title: qsTr("Tools")
        }

        Menu{
            title: qsTr("Help")
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

            label: "T1"
        }

        VolumeControl{
            Layout.preferredWidth: parent.width / 9 / 2
            Layout.fillHeight: true

            label: "T2"
        }

        PlaylistEditor{
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

            label: "WAV"
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
}
