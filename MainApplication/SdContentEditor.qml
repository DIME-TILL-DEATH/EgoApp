import QtCore

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as QmlDialogs

Item{
    id: _root

    ColumnLayout{
        anchors.fill: parent

        RowLayout{
            Layout.fillWidth: true
            Layout.fillHeight: true

            spacing: 5

            Label{
                text: qsTr("Filter:")

                Layout.preferredHeight: parent.height

                verticalAlignment: Label.AlignVCenter
            }

            TextField{

                Layout.preferredWidth: parent.width/3

                onTextEdited: {
                    UiCore.sdContentModel.filterString = text
                }
            }

            Button{
                text: qsTr("Expand all")

                Layout.fillWidth: true

                onClicked: {
                    _treeView.expandRecursively(0)
                }
            }

            Button{
                text: qsTr("Collapse all")

                Layout.fillWidth: true

                onClicked: {
                    _treeView.collapseRecursively(0)
                }
            }
        }

        TreeView{
            id: _treeView

            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 3

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

                MouseArea {
                    anchors.fill: parent

                    acceptedButtons: Qt.RightButton

                    onClicked: (mouse) => {
                        if(mouse.button === Qt.RightButton) {
                            _contextMenu.x = mouse.x
                            _contextMenu.y = mouse.y
                            // _contextMenu.open()
                        }

                    }
                }

                Rectangle{
                    visible: _dropArea.containsDrag

                    anchors.fill: parent

                    color: palette.highlight
                }
            }

            // Connections{
            //     target: UiCore.sdContentModel.sdProxyModel

            //     function onLayoutChanged(parents, hint){
            //         _treeView.expand(0)
            //     }
            // }

            Menu{
                id: _contextMenu

                MenuItem {
                    text: qsTr("Set track 1 link")

                    onTriggered: {

                    }
                }

                MenuItem {
                    text: qsTr("Set track 2 link")
                }
            }
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
            label: qsTr("WAV player") + ((SoundCore.wavPlaying !== "") ? qsTr(", now playing: ") +  SoundCore.wavPlaying : "")

            Layout.preferredWidth: parent.width
            Layout.preferredHeight: parent.height/8

            value: SoundCore.sdTrackPosition
            duration: SoundCore.sdTrackDuration

            onPlayPause: {
                SoundCore.playContent(_itemSelectionModel.currentIndex);
            }

            onStop: {
                SoundCore.stop();
            }

            onMoved: {
                SoundCore.setPosition(SoundPlayer.PlaySd, duration * position)
            }

            Shortcut{
                id: _shortcutPlayEgo

                sequence: "space"

                onActivated: {
                    SoundCore.playContent(_itemSelectionModel.currentIndex);
                }

                Component.onCompleted: {
                    AppGlobals.playWavShortcut = _shortcutPlayEgo.nativeText
                }
            }
        }
    }

    Dialog{
        id: _folderNameDialog

        title: qsTr("Enter folder name")

        width: 300
        height: 100

        contentItem: TextField{
            id: _folderTextField

            validator: RegularExpressionValidator{
                id: _onlyFileNamesValidator

                regularExpression: /^(?!^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$)[^<>:"/\\|?*\x00-\x1F]*[^<>:"/\\|?*\x00-\x1F\s.]$/
            }

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
