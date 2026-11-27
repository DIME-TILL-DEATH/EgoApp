import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: _root

    color: "transparent"

    property alias label: _indicatorLabel.text
    property alias value: _slider.value
    property alias leftEnabled: _chkBoxL.checked
    property alias rightEnabled: _chkBoxR.checked
    property alias muted: _muteButton.muted

    signal sliderValueChanged(var value)
    signal leftMuteChanged(var state)
    signal rightMuteChanged(var state)
    signal trackMuteChanged()

    ColumnLayout{
        anchors.fill: parent

        Slider{
            id: _slider

            orientation: Qt.Vertical

            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.alignment: Text.AlignHCenter

            from: 0.0
            to: 1.0

            value: SoundCore.track1Volume

            onMoved: {
                _root.sliderValueChanged(_slider.value);
            }
        }

        Button{
            id: _muteButton

            icon.source: muted ? "qrc:/icons/sound-off.svg" : "qrc:/icons/sound-on.svg"

            property bool muted: false

            Layout.preferredWidth: parent.width * 2/4
            Layout.alignment: Text.AlignHCenter

            onClicked: {
                _root.trackMuteChanged()
            }
        }

        Label{
            id: _indicatorLabel

            Layout.preferredWidth: parent.width

            horizontalAlignment: Text.AlignHCenter
        }

        Row{
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: _chkBoxL.implicitHeight
            Layout.bottomMargin: 5

            CheckBox{
                id: _chkBoxL

                height: parent.height
                width: parent.width/2

                text: "L"

                checked: true

                contentItem: Label {
                    text: _chkBoxL.text
                    font: _chkBoxL.font
                    // horizontalAlignment: Text.AlignHCenter
                    // verticalAlignment: Text.AlignBottom
                    leftPadding: 4
                    topPadding: _chkBoxR.indicator.height + _chkBoxR.spacing + 5
                }

                onClicked: {
                    _root.leftMuteChanged(_chkBoxL.checked)
                }
            }

            CheckBox{
                id: _chkBoxR

                height: parent.height
                width: parent.width/2

                text: "R"

                checked: true

                contentItem: Label {

                    text: _chkBoxR.text
                    font: _chkBoxR.font
                    // horizontalAlignment: Text.AlignHCenter
                    // verticalAlignment: Text.AlignBottom
                    leftPadding: 4
                    topPadding: _chkBoxR.indicator.height + _chkBoxR.spacing + 5
                }

                onClicked: {
                    _root.rightMuteChanged(_chkBoxR.checked)
                }
            }
        }
    }
}
