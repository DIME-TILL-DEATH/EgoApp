import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: _root

    color: "transparent"

    ColumnLayout{
        anchors.fill: parent

        RowLayout{
            Layout.preferredWidth: parent.width

            Label{
                text: qsTr("Playlist:")
            }

            ComboBox{
                Layout.preferredWidth: _root.width/2
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
            Layout.preferredHeight: parent.height/2
        }

        RowLayout{
            Layout.preferredWidth: parent.width

            Button{
                Layout.fillWidth: true
                Layout.fillHeight: true

                text: qsTr("ADD SONG")
            }

            Button{
                Layout.fillWidth: true
                Layout.fillHeight: true

                text: qsTr("DELETE SONG")
            }

            Button{
                Layout.fillWidth: true
                Layout.fillHeight: true

                text: qsTr("CHECK")
            }
        }

        RowLayout{
            Layout.preferredWidth: parent.width

            Label{
                text: "T1"
            }

            TextField{
                readOnly: true
            }

            CheckBox{
                text: qsTr("Play next")
            }

            Button{
                text: qsTr("Set link")
            }
        }

        RowLayout{
            Layout.preferredWidth: parent.width

            Label{
                text: "T2"
            }

            TextField{
                readOnly: true
            }

            Button{
                text: qsTr("Set link")
            }
        }

        PlayerPanel{
            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 2
        }
    }
}
