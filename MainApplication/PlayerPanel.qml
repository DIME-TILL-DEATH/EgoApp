import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: _root

    border.width: 1
    border.color: _label.color

    color: "transparent"

    property alias label: _label.text

    property alias duration: _slider.to
    property alias value: _slider.value
    property alias position: _slider.position

    // property alias timeLabel: _timeLabel.text

    signal moved()

    signal playPause()
    signal stop()

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
                Layout.preferredHeight: parent.height * 0.7
                Layout.preferredWidth: height

                icon.source: "icons/play.svg"

                onClicked: _root.playPause()
            }

            Button{
                Layout.preferredHeight: parent.height * 0.7
                Layout.preferredWidth: height

                icon.source: "icons/stop.svg"

                onClicked: _root.stop()
            }

            Slider{
                id: _slider

                Layout.fillWidth: true

                onMoved:
                {
                    _root.moved();
                }
            }

            Label{
                id: _timeLabel

                Layout.preferredWidth: parent.width / 8

                text:  Math.trunc(_slider.value / 1000 / 60).toString().padStart(2, '0') + ":"
                       + Math.trunc(_slider.value / 1000 % 60).toString().padStart(2, '0')
            }
        }
    }
}
