import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import CppObjects

Item{
    ColumnLayout{
        anchors.fill: parent

        TreeView{
            id: _treeView

            clip: true
            boundsBehavior: Flickable.StopAtBounds

            model: UiCore.sdContentModel
            rootIndex: UiCore.sdContentModel.rootIndex
            delegate: TreeViewDelegate{}

            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 3
        }

        RowLayout{
            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 1

            Button{
                text: qsTr("ADD FOLDER")

                Layout.fillWidth: true
            }

            Button{
                text: qsTr("ADD WAV")

                Layout.fillWidth: true
            }

            Button{
                text: qsTr("ADD MIDI")

                Layout.fillWidth: true
            }

            Button{
                text: qsTr("DELETE")

                Layout.fillWidth: true
            }
        }

        PlayerPanel{
            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 2
        }
    }
}
