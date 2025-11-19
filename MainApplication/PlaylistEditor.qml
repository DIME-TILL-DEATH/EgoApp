import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// import CppObjects 1.0

Rectangle {
    id: _root

    color: "transparent"

    ColumnLayout{
        anchors.fill: parent

        RowLayout{
            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 1

            Label{
                text: qsTr("Playlist:")
            }

            ComboBox{
                id: _plsCombo

                Layout.preferredWidth: _root.width/2

                model: UiCore.avaliablePlaylists
                currentIndex: UiCore.currentPlaylistIndex

                onActivated:{
                    UiCore.currentPlaylistIndex = _plsCombo.currentIndex
                }
            }

            Button{
                text: "+"

                onClicked: _playlistNameDialog.open()
            }

            Button{
                text: "-"

                enabled: _plsCombo.count > 0

                onClicked: _removePlsConfirmationDialog.open()
            }
        }

        Rectangle{
            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 5

            color: "transparent"

            ListView{

                model: UiCore.currentPlaylist

                clip: true

                anchors.fill: parent
                anchors.margins: 5

                boundsBehavior: Flickable.StopAtBounds

                spacing: 2

                delegate: Rectangle{
                    height: 20
                    width: parent.width

                    color: "transparent"

                    Label{
                        text: (index+1) + ": " + t1Name + " | " + t2Name
                    }
                }

                ScrollBar.vertical: ScrollBar{
                    policy: ScrollBar.AlwaysOn
                }
            }
        }

        RowLayout{
            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 1

            Button{
                Layout.fillWidth: true

                text: qsTr("ADD SONG")
            }

            Button{
                Layout.fillWidth: true

                text: qsTr("DELETE SONG")
            }

            Button{
                Layout.fillWidth: true

                text: qsTr("CHECK")
            }
        }

        CheckBox{
            text: qsTr("Play next")
        }

        RowLayout{
            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 1

            Label{
                text: "T1"
            }

            TextField{
                id: _t1PathFiled

                readOnly: true
                Layout.fillWidth: true
            }

            Button{
                text: qsTr("Set link")

                enabled: UiCore.sdContentModel.canSetTrack
            }
        }

        RowLayout{
            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 1

            Label{
                text: "T2"
            }

            TextField{
                id: _t2PathFiled

                readOnly: true
                Layout.fillWidth: true
            }

            Button{
                text: qsTr("Set link")

                enabled: UiCore.sdContentModel.canSetTrack && _t1PathFiled.text !== ""
            }
        }

        PlayerPanel{
            label: qsTr("EGO player")

            Layout.preferredWidth: parent.width
            Layout.preferredHeight: parent.height/8
        }
    }

    Dialog{
        id: _playlistNameDialog

        title: qsTr("Enter playlist name")

        width: 300
        height: 100

        contentItem: TextField{
            id: _folderTextField

            text: "default"
        }

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
           UiCore.addPlaylist(_folderTextField.text);
        }
    }

    Dialog{
        id: _removePlsConfirmationDialog

        title: qsTr("Remove playlist?")

        width: 250
        height: 100

        contentItem: Label{
            text: qsTr("Do you really want to delete object?")
            horizontalAlignment: Label.AlignHCenter
        }

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
           UiCore.deletePlaylist(_plsCombo.currentIndex);
        }
    }
}
