import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as QmlDialogs

// import CppObjects 1.0

Rectangle {
    id: _root

    color: "transparent"

    property alias hotKeysDialog: _hotKeysDialog

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
                        width: _listView.width - 20

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
                                id: _text

                                text: (index + 1) + ((index != fileNum) ? "(" + fileNum + ".ego)" : "")
                                       + ": " + t1Name
                                       + (t2Name !== "" ? " | " + t2Name : "")
                            }

                            Image{
                                id: _nextImage

                                height: parent.height
                                width: height

                                visible: playNext

                                anchors.left: _text.right

                                source: "icons/next3.svg"
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
                id: _btnAddSong

                Layout.fillWidth: true

                text: qsTr("ADD SONG")

                onClicked: {
                    UiCore.currentPlaylist.insertRows(_listView.currentIndex + 1, 1);
                    _listView.currentIndex = _listView.currentIndex + 1
                }

                Shortcut{
                    id: _shortcutAddSong

                    sequence: "Ctrl+S"

                    onActivated: {
                        if(_btnAddSong.enabled)
                            _btnAddSong.clicked();
                    }
                }
            }

            Button{
                id: _btnDeleteSong

                Layout.fillWidth: true

                text: qsTr("DELETE SONG")

                onClicked: {
                    _removeSongConfirmationDialog.open()
                }

                Shortcut{
                    id: _shortcutDeleteSong

                    sequence: "Ctrl+D"

                    onActivated: {
                        if(_btnDeleteSong.enabled)
                            _btnDeleteSong.clicked();
                    }
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

            onClicked:{
                var indx = UiCore.currentPlaylist.index(_listView.currentIndex, 0);
                UiCore.currentPlaylist.setPlayNext(indx, _cbPlayNext.checked)
            }

            Binding{
                target: _cbPlayNext
                property: "checked"
                value: UiCore.currentPlaylist.data(UiCore.currentPlaylist.index(_listView.currentIndex, 0), PlaylistModel.PlayNextRole)
                restoreMode: Binding.RestoreBinding
            }

            Shortcut{
                id: _shortcutPlayNext

                sequence: "Ctrl+N"

                onActivated: {
                    _cbPlayNext.checked = !_cbPlayNext.checked
                    var indx = UiCore.currentPlaylist.index(_listView.currentIndex, 0);
                    UiCore.currentPlaylist.setPlayNext(indx, _cbPlayNext.checked)
                }
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
                id: _btnSetLinkT1

                text: qsTr("Set link")

                enabled: UiCore.sdContentModel.canSetTrack

                onClicked:{
                    var indx = UiCore.currentPlaylist.index(_listView.currentIndex, 0);
                    UiCore.currentPlaylist.setLink(indx, 0, UiCore.sdContentModel.selectedPath);
                }

                Shortcut{
                    id: _shortcutSetLink1

                    sequence: "1"

                    onActivated: {
                        if(_btnSetLinkT1.enabled)
                            _btnSetLinkT1.clicked();
                    }
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
                id: _btnSetLinkT2

                text: qsTr("Set link")

                enabled: UiCore.sdContentModel.canSetTrack && _t1PathFiled.text !== ""
                onClicked:{
                    var indx = UiCore.currentPlaylist.index(_listView.currentIndex, 0);
                    UiCore.currentPlaylist.setLink(indx, 1, UiCore.sdContentModel.selectedPath);
                }

                Shortcut{
                    id: _shortcutSetLink2

                    sequence: "2"

                    onActivated: {
                        if(_btnSetLinkT2.enabled)
                            _btnSetLinkT2.clicked();
                    }
                }
            }
        }

        PlayerPanel{
            label: qsTr("EGO player")

            Layout.preferredWidth: parent.width
            Layout.preferredHeight: parent.height/8

            value: SoundCore.egoTrackPosition
            duration: SoundCore.egoTrackDuration

            onPlayPause: {
                var indx = UiCore.currentPlaylist.index(_listView.currentIndex, 0);
                SoundCore.playEgo(indx);
            }

            onStop: {
                SoundCore.stop();
            }

            onMoved: {
                SoundCore.setPosition(SoundPlayer.PlayEgo, duration * position)
            }

            Shortcut{
                id: _shortcutPlayEgo

                sequence: "Ctrl+space"

                onActivated: {
                    var indx = UiCore.currentPlaylist.index(_listView.currentIndex, 0);
                    SoundCore.playEgo(indx);
                }
            }
        }
    }

    Dialog{
        id: _playlistNameDialog

        title: qsTr("Enter playlist name")

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
           UiCore.addPlaylist(_folderTextField.text);
        }
    }

    QmlDialogs.MessageDialog{
        id: _removePlsConfirmationDialog

        title: qsTr("Remove playlist?")

        text: qsTr("Do you really want to delete playlist?")

        buttons: QmlDialogs.MessageDialog.Ok
                | QmlDialogs.MessageDialog.Cancel

        onAccepted: {
           UiCore.deletePlaylist(_plsCombo.currentIndex);
        }
    }

    QmlDialogs.MessageDialog{
        id: _removeSongConfirmationDialog

        title: qsTr("Remove song?")

        text: qsTr("Do you really want to delete song?")


        buttons: QmlDialogs.MessageDialog.Ok
                | QmlDialogs.MessageDialog.Cancel

        onAccepted: {
            UiCore.currentPlaylist.removeRows(_listView.currentIndex, 1);
            _listView.currentIndex = _listView.currentIndex - 1
        }
    }

    QmlDialogs.MessageDialog{
        id: _hotKeysDialog

        title: qsTr("Hotkeys")

        text: qsTr("Add song: ") + _shortcutAddSong.nativeText + "\n" +
              qsTr("Delete song: ") + _shortcutDeleteSong.nativeText + "\n" +
              qsTr("Set track1 link: ") + _shortcutSetLink1.nativeText + "\n" +
              qsTr("Set track2 link: ") + _shortcutSetLink2.nativeText + "\n" +
              qsTr("Toggle play next: ") + _shortcutPlayNext.nativeText + "\n" +
              qsTr("Play EGO song: ") + _shortcutPlayEgo.nativeText + "\n" +
              qsTr("Play WAV file: ") + AppGlobals.playWavShortcut
    }
}
