import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import CppObjects

ApplicationWindow {
    width: 800
    height: 400
    visible: true
    title: qsTr("EgoGig")

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
}
