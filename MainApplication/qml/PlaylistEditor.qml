import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as QmlDialogs


Rectangle {
    id: _root

    color: "transparent"

    property bool currentPlaylistCorrupted: UiCore.currentPlaylist !== null
                                           && UiCore.currentPlaylist.corrupted
    property bool hasSelectedSong: UiCore.currentPlaylist !== null
                                   && _listView.currentIndex >= 0
                                   && _listView.currentIndex < UiCore.currentPlaylist.rowCount()

    function updatePlayNext(state) {
        var playlist = UiCore.currentPlaylist;
        if (playlist === null || !hasSelectedSong)
            return;

        var songIndex = playlist.index(_listView.currentIndex, 0);
        if (!playlist.setPlayNext(songIndex, state))
            _cbPlayNext.checked = playlist.data(songIndex, PlaylistModel.PlayNextRole);
    }

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
                opacity: _root.currentPlaylistCorrupted ? 0.45 : 1

                delegate: ItemDelegate {
                    required property int index
                    required property string modelData

                    width: _plsCombo.width
                    text: modelData
                    highlighted: _plsCombo.highlightedIndex === index
                    opacity: UiCore.currentPlaylistIndex === index
                             && UiCore.currentPlaylist !== null
                             && UiCore.currentPlaylist.corrupted ? 0.45 : 1
                }

                onActivated:{
                    UiCore.currentPlaylistIndex = _plsCombo.currentIndex
                }
            }

            Button{
                text: "+"

                Layout.fillWidth: true

                onClicked: _playlistNameDialog.open()
            }

            Button{
                text: "-"

                Layout.fillWidth: true

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
                enabled: !_root.currentPlaylistCorrupted
                opacity: _root.currentPlaylistCorrupted ? 0.45 : 1

                boundsBehavior: Flickable.StopAtBounds

                spacing: 2

                DropArea{
                    anchors.fill: parent

                    z: -5

                    onDropped: function (drag) {
                        // handling drop cancel
                        var from = drag.source.visualIndex;
                        var to = (drag.source as Item).dragParent.modelIndex;
                        _visualModel.items.move(from, to);
                    }
                }

                model: DelegateModel{
                    id: _visualModel

                    model: UiCore.currentPlaylist


                    delegate: DropArea{
                        id: _delegateRoot

                        height: 20
                        width: _listView.width - 20

                        property int modelIndex
                        property int visualIndex: DelegateModel.itemsIndex

                        property string trackT1Path: t1Path
                        property string trackT2Path: t2Path
                        property bool trackT1Missing: t1Missing
                        property bool trackT2Missing: t2Missing


                        onEntered: function (drag) {
                            if(drag.source.Drag.active){
                                var from = drag.source.visualIndex;
                                var to = _thing.visualIndex;
                                _visualModel.items.move(from, to);
                            }
                        }

                        onDropped: function (drag) {
                            var from = (drag.source as Item).dragParent.modelIndex;
                            var to = (drag.source as Item).visualIndex;
                            if (!UiCore.currentPlaylist.moveSong(from, to))
                                _visualModel.items.move(to, from);
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

                                drag.minimumY: _listView.y
                                drag.maximumY: _listView.y +_listView.height - height
                                               // (_listView.contentHeight < _listView.height ? _listView.contentHeight : _listView.height)
                                               //      - height

                                onPressed: (mouse) =>
                                {
                                    _listView.currentIndex = index
                                    _delegateRoot.modelIndex = _thing.visualIndex
                                }

                                onReleased:
                                {
                                    _thing.Drag.drop()
                                    _listView.currentIndex = index
                                }

                                onPositionChanged: (mouse) =>
                                {
                                    if(!_slowDownTimer.running){
                                        if(mapToItem(_listView ,mouse.x, mouse.y).y < _listView.y && _listView.contentY > _delegateRoot.height)
                                        {
                                            _listView.contentY = _listView.contentY - _delegateRoot.height
                                        }

                                        if(mapToItem(_listView ,mouse.x, mouse.y).y > _listView.y + _listView.height - height
                                           && _listView.contentY < _listView.contentHeight - _listView.height - height)
                                        {
                                           _listView.contentY = _listView.contentY + _delegateRoot.height
                                        }

                                        _slowDownTimer.start();
                                    }
                                }

                                Timer{
                                    id: _slowDownTimer

                                    interval: 10
                                }
                            }

                            Row{
                                width: parent.width
                                height: parent.height

                                Label{
                                    id: _text

                                    width: parent.width
                                           - (_midiImage.visible ? _midiImage.width : 0)
                                           - (_nextImage.visible ? _nextImage.width : 0)
                                    color: fileMissing ? "red" : palette.text
                                    elide: Text.ElideMiddle
                                    wrapMode: Text.NoWrap
                                    clip: true
                                    verticalAlignment: Text.AlignVCenter

                                    text: (index + 1) + ((index != fileNum) ? "(" + fileNum + ".ego)" : "")
                                           + ": " + (t1Name !== "" ? t1Name : t2Name)
                                           + (t1Name !== "" && t2Name !== "" ? " | " + t2Name : "")
                                }

                                Image{
                                    id: _midiImage

                                    height: parent.height
                                    width: height

                                    visible: midiExist

                                    source: "qrc:/icons/midi.svg"
                                }

                                Image{
                                    id: _nextImage

                                    height: parent.height
                                    width: height

                                    visible: playNext

                                    source: "qrc:/icons/next.svg"
                                }
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
                enabled: !_root.currentPlaylistCorrupted && UiCore.currentPlaylist !== null
                opacity: _root.currentPlaylistCorrupted ? 0.45 : 1

                onClicked: {
                    var playlist = UiCore.currentPlaylist;
                    if (playlist === null)
                        return;

                    var insertPosition = Math.max(0, _listView.currentIndex + 1);
                    if (playlist.insertRows(insertPosition, 1))
                        _listView.currentIndex = insertPosition;
                }

                Shortcut{
                    id: _shortcutAddSong

                    sequence: "Ctrl+S"

                    onActivated: {
                        if(_btnAddSong.enabled)
                            _btnAddSong.clicked();
                    }

                    Component.onCompleted: {
                        AppGlobals.addSongShortcut = _shortcutAddSong.nativeText
                    }
                }
            }

            Button{
                id: _btnDeleteSong

                Layout.fillWidth: true

                text: qsTr("DELETE SONG")

                enabled: !_root.currentPlaylistCorrupted && _root.hasSelectedSong

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

                    Component.onCompleted: {
                        AppGlobals.deleteSongShortcut = _shortcutDeleteSong.nativeText
                    }
                }
            }

            Button{
                Layout.fillWidth: true

                text: qsTr("CHECK")

                enabled: UiCore.currentPlaylist !== null

                onClicked: {
                    UiCore.checkPlaylist(_plsCombo.currentText)
                }
            }
        }

        CheckBox{
            id: _cbPlayNext

            text: qsTr("Play next")
            enabled: !_root.currentPlaylistCorrupted && _root.hasSelectedSong
            opacity: _root.currentPlaylistCorrupted ? 0.45 : 1

            onClicked: _root.updatePlayNext(_cbPlayNext.checked)

            Binding{
                target: _cbPlayNext
                property: "checked"
                value: _root.hasSelectedSong
                       ? UiCore.currentPlaylist.data(UiCore.currentPlaylist.index(_listView.currentIndex, 0), PlaylistModel.PlayNextRole)
                       : false
                restoreMode: Binding.RestoreBinding
            }

            Shortcut{
                id: _shortcutPlayNext

                sequence: "Ctrl+N"

                onActivated: {
                    if (!_cbPlayNext.enabled || !_root.hasSelectedSong)
                        return;
                    var playlist = UiCore.currentPlaylist;
                    var songIndex = playlist.index(_listView.currentIndex, 0);
                    _root.updatePlayNext(!playlist.data(songIndex, PlaylistModel.PlayNextRole))
                }

                Component.onCompleted: {
                    AppGlobals.togglePlayNextShortcut = _shortcutPlayNext.nativeText
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

                text: _listView.currentItem ? _listView.currentItem.trackT1Path : ""
                color: _listView.currentItem && _listView.currentItem.trackT1Missing ? "red" : palette.text
            }

            Button{
                id: _btnSetLinkT1

                text: qsTr("Set link")

                enabled: !_root.currentPlaylistCorrupted && _root.hasSelectedSong
                         && UiCore.sdContentModel.canSetTrack
                opacity: _root.currentPlaylistCorrupted ? 0.45 : 1

                onClicked:{
                    if (!_root.hasSelectedSong)
                        return;
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

                    Component.onCompleted: {
                        AppGlobals.setLink1Shortcut = _shortcutSetLink1.nativeText
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

                text: _listView.currentItem ? _listView.currentItem.trackT2Path : ""
                color: _listView.currentItem && _listView.currentItem.trackT2Missing ? "red" : palette.text
            }

            Button{
                text: qsTr("Clear")
                enabled: !_root.currentPlaylistCorrupted && _root.hasSelectedSong
                opacity: _root.currentPlaylistCorrupted ? 0.45 : 1
                onClicked: {
                    if (!_root.hasSelectedSong)
                        return;
                    var indx = UiCore.currentPlaylist.index(_listView.currentIndex, 0);
                    UiCore.currentPlaylist.setLink(indx, 1, "");
                }
            }

            Button{
                id: _btnSetLinkT2

                text: qsTr("Set link")

                enabled: !_root.currentPlaylistCorrupted && _root.hasSelectedSong
                         && UiCore.sdContentModel.canSetTrack
                opacity: _root.currentPlaylistCorrupted ? 0.45 : 1
                onClicked:{
                    if (!_root.hasSelectedSong)
                        return;
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

                    Component.onCompleted: {
                        AppGlobals.setLink2Shortcut = _shortcutSetLink2.nativeText
                    }
                }
            }   
        }

        PlayerPanel{
            enabled: !_root.currentPlaylistCorrupted && _root.hasSelectedSong
            opacity: _root.currentPlaylistCorrupted ? 0.45 : 1

            label: qsTr("EGO player") + ((SoundCore.egoSongPlaying !== "") ? qsTr(", now playing: ") +  SoundCore.egoSongPlaying : "")

            Layout.preferredWidth: parent.width
            Layout.preferredHeight: parent.height/8

            value: SoundCore.egoTrackPosition
            duration: SoundCore.egoTrackDuration
            playbackActive: SoundCore.egoPlaybackActive

            onPlayPause: {
                if (_root.currentPlaylistCorrupted || UiCore.currentPlaylist === null)
                    return;
                var indx = UiCore.currentPlaylist.index(_listView.currentIndex, 0);
                SoundCore.PlayPause(SoundPlayer.PlayEgo, indx);
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
                    if (_root.currentPlaylistCorrupted || UiCore.currentPlaylist === null)
                        return;
                    var indx = UiCore.currentPlaylist.index(_listView.currentIndex, 0);
                    SoundCore.PlayPause(SoundPlayer.PlayEgo, indx);
                }

                Component.onCompleted: {
                    AppGlobals.playEgoShortcut = _shortcutPlayEgo.nativeText
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

                regularExpression: /^[a-zA-Z0-9][a-zA-Z0-9_\-\.\s\(\)]*$/
            }


            text: "default"
        }

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
           UiCore.addPlaylist(_folderTextField.text);
        }
    }

    QmlDialogs.MessageDialog{
        id: _brokenPlaylistDialog

        title: qsTr("Damaged playlist")
        text: qsTr("This playlist contains invalid or unreadable .ego files or incorrect .ego numbering. Editing and playback are disabled until the playlist files are fixed.")
        buttons: QmlDialogs.MessageDialog.Ok
    }

    Connections {
        target: UiCore

        function onCurrentPlaylistChanged() {
            if (UiCore.currentPlaylist !== null && UiCore.currentPlaylist.corrupted)
                _brokenPlaylistDialog.open()
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
            var removedRow = _listView.currentIndex;
            if (UiCore.currentPlaylist.removeRows(removedRow, 1))
                _listView.currentIndex = Math.min(removedRow,
                                                  UiCore.currentPlaylist.rowCount() - 1)
        }
    }
}
