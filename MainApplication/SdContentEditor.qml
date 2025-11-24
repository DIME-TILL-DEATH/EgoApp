import QtCore

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as QmlDialogs

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

                onCurrentChanged:{
                    UiCore.sdContentModel.currentSelectionChanged(currentIndex)
                }
            }

            delegate: TreeViewDelegate{
                id: _delegate

                DropArea{
                    id: _dropArea

                    anchors.fill: parent

                    onDropped: (drop) => {
                        var currentIndex = _treeView.index(_delegate.row, _delegate.column);

                        UiCore.sdContentModel.addContent(currentIndex, drop.urls);
                    }
                }

                Rectangle{
                    visible: _dropArea.containsDrag

                    anchors.fill: parent

                    color: palette.highlight
                }
            }

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

                onClicked: _wavFileDialog.open()
            }

            Button{
                text: qsTr("ADD MIDI")

                Layout.fillWidth: true

                onClicked: _midiFileDialog.open()
            }

            Button{
                text: qsTr("DELETE")

                Layout.fillWidth: true

                onClicked: _removeConfirmationDialog.open()
            }
        }

        PlayerPanel{
            label: qsTr("WAV player")

            Layout.preferredWidth: parent.width
            Layout.preferredHeight: parent.height/8
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

    QmlDialogs.FileDialog{
        id: _wavFileDialog

        fileMode: QmlDialogs.FileDialog.OpenFiles

        nameFilters: ["WAV(*.wav)", "MP3(*.mp3)"]

        onAccepted: {
            UiCore.sdContentModel.addContent(_itemSelectionModel.currentIndex, _wavFileDialog.selectedFiles);
        }

        Settings{
            property alias path: _wavFileDialog.currentFolder
        }
    }

    QmlDialogs.FileDialog{
        id: _midiFileDialog

        fileMode: QmlDialogs.FileDialog.OpenFiles

        nameFilters: ["Midi files (*.mid)"]

        onAccepted: {
            UiCore.sdContentModel.addContent(_itemSelectionModel.currentIndex, _midiFileDialog.selectedFiles);
        }

        Settings{
            property alias path: _wavFileDialog.currentFolder
        }
    }

    QmlDialogs.MessageDialog{
        id: _removeConfirmationDialog

        title: qsTr("Remove item?")

        text: qsTr("Do you really want to delete object?")

        buttons: QmlDialogs.MessageDialog.Ok
                | QmlDialogs.MessageDialog.Cancel

        onAccepted: {
           UiCore.sdContentModel.deleteObject(_itemSelectionModel.currentIndex)
        }
    }
}
