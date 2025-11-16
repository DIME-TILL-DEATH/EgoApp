import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: _root

    border.width: 1
    border.color: _label.color

    color: "transparent"

    property alias label: _label.text

    signal playPause()
    signal stop

    Column{
        anchors.fill: parent

        Label{
            id: _label

            x: 5

            text: "Label Player"
        }

        RowLayout{
            id: _layout

            width: parent.width - x*2
            height: parent.height * 2/3

            x: 5

            Button{
                Layout.preferredWidth: height

                onClicked: _root.playPause()
            }

            Button{
                Layout.preferredWidth: height

                onClicked: _root.stop
            }

            Slider{

            }

            Label{
                text: "0.0 s"
            }
        }
    }
}
