import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as QmlDialogs

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
                                    console.log(_thing.Drag)
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


                                    text: (index + 1) + ((index != fileNum) ? "(" + fileNum + ".ego)" : "")
                                           + ": " + t1Name
                                           + (t2Name !== "" ? " | " + t2Name : "")
                                            // + (midiExist ? "|have midi file" : "")
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

                    Component.onCompleted: {
                        AppGlobals.addSongShortcut = _shortcutAddSong.nativeText
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

                    Component.onCompleted: {
                        AppGlobals.deleteSongShortcut = _shortcutDeleteSong.nativeText
                    }
                }
            }

            Button{
                Layout.fillWidth: true

                text: qsTr("CHECK")

                onClicked: {
                    UiCore.checkPlaylists()
                }
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

                    Component.onCompleted: {
                        AppGlobals.setLink2Shortcut = _shortcutSetLink2.nativeText
                    }
                }
            }

            Button{
                text: qsTr("Clear")
                onClicked: {
                    var indx = UiCore.currentPlaylist.index(_listView.currentIndex, 0);
                    UiCore.currentPlaylist.setLink(indx, 1, "");
                }
            }
        }

        PlayerPanel{
            label: qsTr("EGO player") + ((SoundCore.egoSongPlaying !== "") ? qsTr(", now playing: ") +  SoundCore.egoSongPlaying : "")

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
}
