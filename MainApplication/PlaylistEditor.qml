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
                id: _listView
                clip: true

                anchors.fill: parent
                anchors.margins: 5

                boundsBehavior: Flickable.StopAtBounds

                spacing: 2

                model: DelegateModel{
                    id: _visualModel

                    model: UiCore.currentPlaylist

                    delegate: DropArea{
                        id: _delegateRoot

                        height: 20
                        width: _listView.width

                        property int modelIndex
                        property int visualIndex: DelegateModel.itemsIndex

                        onEntered: function (drag) {
                            var from = drag.source.visualIndex;
                            var to = _thing.visualIndex;
                            _visualModel.items.move(from, to);
                        }

                        onDropped: function (drag) {
                            var from = modelIndex;
                            var to = (drag.source as Item).visualIndex;
                            UiCore.currentPlaylist.moveSong(from, to);
                        }

                        Rectangle{
                            id: _thing

                            width: _delegateRoot.width
                            height: _delegateRoot.height

                            color: "transparent"

                            property Item dragParent: _delegateRoot
                            property int visualIndex: _delegateRoot.visualIndex

                            Drag.active: _mouseAreaDrag.drag.active
                            Drag.source: _thing
                            Drag.hotSpot.x: width / 2
                            Drag.hotSpot.y: height / 2

                            opacity: Drag.active ? 0.5 : 1

                            MouseArea {
                                id: _mouseAreaDrag
                                anchors.fill: parent

                                drag.target: _thing
                                drag.axis: Drag.YAxis

                                onPressed:
                                {
                                    _listView.currentIndex = index
                                    _delegateRoot.modelIndex = visualIndex
                                }
                                onReleased: _thing.Drag.drop()
                            }

                            Label{
                                text: (index + 1) + ((index != fileNum) ? "(" + fileNum + ".ego)" : "")
                                       + ": " + t1Name
                                       + (t2Name !== "" ? " | " + t2Name : "")
                                       + (playNext ? " |->" : "")

                            }

                            states: State {
                                when: _mouseAreaDrag.drag.active

                                ParentChange {
                                    target: _thing
                                    parent: _listView
                                }

                                AnchorChanges {
                                    target: _thing
                                    anchors.horizontalCenter: undefined
                                    anchors.verticalCenter: undefined
                                }
                            }
                        }
                    }
                }

                highlightMoveDuration: 100
                highlight: Rectangle{
                    color: palette.highlight
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

                onClicked: {
                    UiCore.currentPlaylist.insertRows(_listView.currentIndex + 1, 1);
                    _listView.currentIndex = _listView.currentIndex + 1
                }
            }

            Button{
                Layout.fillWidth: true

                text: qsTr("DELETE SONG")

                onClicked: {
                    _removeSongConfirmationDialog.open()
                }
            }

            Button{
                Layout.fillWidth: true

                text: qsTr("CHECK")
            }
        }

        CheckBox{
            id: _cbPlayNext

            text: qsTr("Play next")

            checked: UiCore.currentPlaylist.data(UiCore.currentPlaylist.index(_listView.currentIndex, 0), PlaylistModel.PlayNextRole)

            onClicked:{
                var indx = UiCore.currentPlaylist.index(_listView.currentIndex, 0);
                UiCore.currentPlaylist.setPlayNext(indx, _cbPlayNext.checked)
            }
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

                Connections{
                    target: UiCore.currentPlaylist

                    function onDataChanged(){
                        _t1PathFiled.text = UiCore.currentPlaylist.data(UiCore.currentPlaylist.index(_listView.currentIndex, 0), PlaylistModel.T1PathRole)
                    }
                }
                Binding{
                    target: _t1PathFiled
                    property: "text"
                    value: UiCore.currentPlaylist.data(UiCore.currentPlaylist.index(_listView.currentIndex, 0), PlaylistModel.T1PathRole)
                    restoreMode: Binding.RestoreBinding
                }
            }

            Button{
                text: qsTr("Set link")

                enabled: UiCore.sdContentModel.canSetTrack

                onClicked:{
                    var indx = UiCore.currentPlaylist.index(_listView.currentIndex, 0);
                    UiCore.currentPlaylist.setLink(indx, 0, UiCore.sdContentModel.selectedPath);
                }
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

                Connections{
                    target: UiCore.currentPlaylist

                    function onDataChanged(){
                        _t2PathFiled.text = UiCore.currentPlaylist.data(UiCore.currentPlaylist.index(_listView.currentIndex, 0), PlaylistModel.T2PathRole)
                    }
                }
                Binding{
                    target: _t2PathFiled
                    property: "text"
                    value: UiCore.currentPlaylist.data(UiCore.currentPlaylist.index(_listView.currentIndex, 0), PlaylistModel.T2PathRole)
                    restoreMode: Binding.RestoreBinding
                }
            }

            Button{
                text: qsTr("Set link")

                enabled: UiCore.sdContentModel.canSetTrack && _t1PathFiled.text !== ""
                onClicked:{
                    var indx = UiCore.currentPlaylist.index(_listView.currentIndex, 0);
                    UiCore.currentPlaylist.setLink(indx, 1, UiCore.sdContentModel.selectedPath);
                }
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

    Dialog{
        id: _removeSongConfirmationDialog

        title: qsTr("Remove song?")

        width: 250
        height: 100

        contentItem: Label{
            text: qsTr("Do you really want to delete object?")
            horizontalAlignment: Label.AlignHCenter
        }

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            UiCore.currentPlaylist.removeRows(_listView.currentIndex, 1);
            _listView.currentIndex = _listView.currentIndex - 1
        }
    }
}
