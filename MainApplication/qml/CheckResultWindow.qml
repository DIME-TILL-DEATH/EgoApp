import QtQuick
import QtQuick.Controls

Window{
    id: _resultWindow

    property alias textResult: _listResults.model

    title: qsTr("Playlist check / loading results")

    Column{
        anchors.fill: parent
        anchors.margins: 10

        spacing: 5
        Rectangle{

            width: parent.width
            height: parent.height * 0.9

            color: "transparent"

            border.width: 1
            border.color: palette.text

            ListView{
                id: _listResults

                anchors.fill: parent
                anchors.margins: 10

                ScrollBar.vertical: ScrollBar{
                    policy: ScrollBar.AlwaysOn
                }

                delegate: Label{
                    width: _listResults.width - 20
                    text: modelData
                    wrapMode: Text.WordWrap
                }
            }

        }

        Button{
            text: qsTr("Ok")

            anchors.horizontalCenter: parent.horizontalCenter

            onClicked: {
                _resultWindow.close()
            }
        }
    }

    Connections{
        target: UiCore

        function onCheckingPlaylistsFinished(result)
        {
            _resultWindow.textResult = result
            _resultWindow.show()
        }

        function onPlaylistLoadIssuesFound(issues)
        {
            _resultWindow.textResult = issues
            _resultWindow.show()
        }
    }
}
