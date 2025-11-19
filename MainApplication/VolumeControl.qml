import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "transparent"

    // border.color: "blue"
    // border.width: 1

    property alias label: _indicatorLabel.text

    ColumnLayout{
        anchors.fill: parent

        Slider{
            id: _slider

            orientation: Qt.Vertical

            Layout.preferredWidth: parent.width
            Layout.preferredHeight: parent.height * 3/4
            Layout.alignment: Text.AlignHCenter
        }

        Button{
            id: _muteButton

            text: "M"

            Layout.preferredWidth: parent.width * 3/4
            Layout.alignment: Text.AlignHCenter
            Layout.verticalStretchFactor: 2
            Layout.fillHeight: true
        }

        Label{
            id: _indicatorLabel

            Layout.preferredWidth: parent.width
            Layout.verticalStretchFactor: 1
            Layout.fillHeight: true

            horizontalAlignment: Text.AlignHCenter
        }

        Row{
            Layout.preferredWidth: parent.width
            Layout.verticalStretchFactor: 2
            Layout.fillHeight: true
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
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    topPadding: _chkBoxL.indicator.height + _chkBoxL.spacing + 2
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
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    topPadding: _chkBoxR.indicator.height + _chkBoxR.spacing + 2
                }
            }
        }
    }
}
