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
                Layout.preferredWidth: _root.width/2

                model: UiCore.avaliablePlaylists
            }

            Button{
                Layout.fillWidth: true
                Layout.fillHeight: true

                text: "+"
            }

            Button{
                Layout.fillWidth: true
                Layout.fillHeight: true

                text: "-"
            }
        }

        ListView{
            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 5
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
                readOnly: true
                Layout.fillWidth: true
            }

            Button{
                text: qsTr("Set link")
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
                readOnly: true
                Layout.fillWidth: true
            }

            Button{
                text: qsTr("Set link")
            }
        }

        PlayerPanel{
            label: qsTr("EGO player")

            Layout.preferredWidth: parent.width
            Layout.preferredHeight: parent.height/8
        }
    }
}
