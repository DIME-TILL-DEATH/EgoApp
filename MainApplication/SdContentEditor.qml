import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// import CppObjects

Item{
    id: _root

    ColumnLayout{
        anchors.fill: parent

        TreeView{
            id: _treeView

            clip: true
            boundsBehavior: Flickable.StopAtBounds
            rootIndex: UiCore.sdContentModel.rootIndex

            model: UiCore.sdContentModel.sdProxyModel


            selectionModel: ItemSelectionModel{
                id: _itemSelectionModel
            }
            delegate: TreeViewDelegate{}

            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 3

            // Connections{
            //     target: UiCore.sdContentModel.sdProxyModel

            //     function onLayoutChanged(parents, hint){
            //         _treeView.expand(0)
            //     }
            // }
        }

        RowLayout{
            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 1

            Button{
                text: qsTr("ADD FOLDER")

                Layout.fillWidth: true

                onClicked: _folderNameDialog.open()
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
            Layout.preferredHeight: parent.height/5
        }
    }

    Dialog{
        id: _folderNameDialog

        title: qsTr("Enter folder name")

        width: 300
        height: 100

        contentItem: TextField{
            id: _folderTextField

            text: "default"
        }

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            UiCore.sdContentModel.addFolder(_itemSelectionModel.currentIndex, _folderTextField.text);
        }
    }
}
