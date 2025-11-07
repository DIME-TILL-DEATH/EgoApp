import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQml.Models 2.11
import QtQuick.Dialogs
import Qt.labs.platform 1.0 as Labs
import QtCore

ApplicationWindow
{
    id: main
    property bool lightVer: true

    property int fileCount: 0
    property int fileOkCount: 0

    property bool wait: false

    property bool afterConvert: false

    visible: true
    width: 720
    height: 480
    title: qsTr("IR converter")

    Settings
    {
        category: "General"
        property alias x: main.x
        property alias y: main.y
        property alias width: main.width
        property alias height: main.height
        property alias fileFolder: fileDialog.currentFolder
        property alias destFolder: folderDialog.currentFolder
        property alias filePrefix: tfPrefix.text
        property alias fileSuffix: tfSuffix.text
        property alias maxFileSize: tfMaxFileSize.text
        property alias sampleRate: cbSampleRate.currentIndex
        property alias chanels:    cbChanels.currentIndex
        property alias bitRate:    cbBitRate.currentIndex
    }

    Column
    {
        id: sourceColumn
        height: parent.height
        width: parent.width*0.95
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 2

        Item
        {
            id: sourceButtonItem
            height: parent.height/10*1
            width: parent.width
            Button
            {
                id: btSource
                anchors.centerIn: parent
                height: parent.height/2
                width: parent.width/2
                onClicked:
                {
                    fileDialog.visible = true
                    if(afterConvert)
                    {
                        afterConvert = false;
                        sourceModel.clear();
                    }
                }
                text: "Select source IR files"
                background: Rectangle
                {
                    id: recSource
                    implicitWidth: 90
                    implicitHeight: 30
                    opacity: enabled ? 1 : 0.3
                    border.color: btSource.down ? "#FA8072" : "#696969"
                    border.width: 2
                    radius: 4
                }
            }
        }


        Rectangle
        {
            id: sourceItem
            height: parent.height/10*6
            width: parent.width
            color: "lightgray"
            radius: 5

            ListView
            {
                id: sourceList
                anchors.fill: parent
                flickableDirection: Flickable.AutoFlickIfNeeded
                clip: true
                contentWidth:  width*2
                model: sourceModel
                ScrollBar.vertical: ScrollBar {active: true}
                ScrollBar.horizontal: ScrollBar {active: true}
                highlight: Rectangle {color: "lightsteelblue"; radius: 3}
                Component
                {
                    id: sourceDelegate
                    Text
                    {
                        text: fileName
                        MouseArea
                        {
                            anchors.fill: parent
                            onClicked:
                            {
                                sourceList.currentIndex = index
                            }
                        }
                    }
                }
                delegate: sourceDelegate
            }
        }

        Rectangle
        {
            id: destItem
            height: parent.height/10*1
            width: parent.width
            color: "lightgray"
            radius: 5

            Row
            {
                anchors.fill: parent
                Item
                {
                    width: parent.width/20*1
                    height: parent.height
                }
                Item
                {
                    width: parent.width/20*13
                    height: parent.height
                    TextEdit
                    {
                        id: destFolder
                        width: parent.width
                        anchors.verticalCenter: parent.verticalCenter
                        clip: true
                    }
                }
                Item
                {
                    width: parent.width/20*1
                    height: parent.height
                }
                Button
                {
                    id: btDest
                    height: parent.height/2
                    width: parent.width/20*4
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: folderDialog.visible = true
                    text: "Select destination folder"
                    background: Rectangle
                    {
                        id: recDest
                        implicitWidth: 90
                        implicitHeight: 30
                        opacity: enabled ? 1 : 0.3
                        border.color: btDest.down ? "#FA8072" : "#696969"
                        border.width: 2
                        radius: 4
                    }
                }
                Item
                {
                    width: parent.width/20*1
                    height: parent.height
                }
            }
        }

        Rectangle
        {
            id: paramItem
            height: parent.height/10*1
            width: parent.width

            color: "lightgray"
            radius: 5
            Row
            {
                anchors.fill: parent
                spacing: 5
                leftPadding: 5
                Text
                {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Prefix:")
                }
                TextField
                {
                    id: tfPrefix
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width/15 //60
                    text: "_"
                }
                Text
                {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Suffix:")
                }
                TextField
                {
                    id: tfSuffix
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width/15//60
                    text: "_"
                }
                Text
                {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Max files size:")
                    visible: !lightVer
                }
                TextField
                {
                    id: tfMaxFileSize
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width/12 //80
                    text: "30000"
                    visible: !lightVer
                }
                Text
                {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Sample rate:")
                    visible: !lightVer
                }
                ComboBox
                {
                    id: cbSampleRate
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width/8 //100
                    currentIndex: 1
                    visible: !lightVer
                    model: ListModel
                    {
                        ListElement { Text: "44100" }
                        ListElement { Text: "48000" }
                    }
                }

                Text
                {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Channels:")
                    visible: !lightVer
                }
                ComboBox
                {
                    id: cbChanels
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width/12 //60
                    currentIndex: 0
                    visible: !lightVer
                    model: ListModel
                    {
                        ListElement { Text: "1" }
                        ListElement { Text: "2" }
                    }
                }

                Text
                {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Bits:")
                    visible: !lightVer
                }
                ComboBox
                {
                    id: cbBitRate
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width/10 //70
                    currentIndex: 1
                    visible: !lightVer
                    model: ListModel
                    {
                        ListElement { Text: "16" }
                        ListElement { Text: "24" }
                        ListElement { Text: "32" }
                    }
                }
            }
        }

        Button
        {
            id: btConvert

            height: parent.height/10*1*0.75
            width: parent.width
            anchors.horizontalCenter : parent.horizontalCenter
            text: "Start conversion "
            enabled: sourceModel.count > 0

            background: Rectangle
            {
                id: recConvert
                implicitWidth: 90
                implicitHeight: 30
                opacity: enabled ? 1 : 0.3
                border.color: btConvert.down ? "#FA8072" : "#696969"
                border.width: 2
                radius: 4
            }
            onClicked:
            {
                var destPath;
                var filesCount = sourceModel.rowCount();
                var fileList = [];
                fileList.length = filesCount;

                for(var i=0; i < filesCount; i++)
                {
                    fileList[i] = sourceModel.get(i).fileName;
                }

                fileOkCount = prBar.value = 0;
                fileCount = prBar.to = filesCount;

                if(_core.setDestinationPath(destFolder.text))
                {
                    _core.setOutputFormat(cbSampleRate.currentText,
                                          cbBitRate.currentText,
                                          cbChanels.currentText,
                                          tfMaxFileSize.text);
                    _core.setPrefixSuffix(tfPrefix.text, tfSuffix.text);
                    _core.startConvert(fileList);
                    main.wait = true;
                }
            }
        }
    }


    FileDialog
    {
        id: fileDialog

        visible: false
        title:  "Choose a file"
        fileMode: FileDialog.OpenFiles
        nameFilters: [ "WAV files (*.wav)", "All files (*.*)" ]

        onAccepted:
        {
            for (var i = 0; i < currentFiles.length; ++i)
            {
                var filePresent = false;
                var cleanPath = currentFiles[i].toString();
                cleanPath = (Qt.platform.os==="windows") ? decodeURIComponent(cleanPath.replace(/^(file:\/{3})|(qrc:\/{2})|(http:\/{2})/,""))
                                                        : decodeURIComponent(cleanPath.replace(/^(file:\/{2})|(qrc:\/{2})|(http:\/{2})/,""));

                for(var j=0; j<sourceModel.rowCount(); j++)
                {
                    if(cleanPath === sourceModel.get(j).fileName)
                    {
                        filePresent = true;
                    }
                }
                if(filePresent === false)
                {
                    sourceModel.append({"fileName": cleanPath});
                }
            }
        }
    }

    FolderDialog
    {
        id: folderDialog
        visible: false
        title:  "Choose a dest folder"

        currentFolder: Labs.StandardPaths.writableLocation(Labs.StandardPaths.DocumentsLocation) + "/AMT/pangaeaCPPA/convertedIR"

        onAccepted:
        {
            var cleanPath = currentFolder.toString();
            cleanPath = (Qt.platform.os==="windows") ? decodeURIComponent(cleanPath.replace(/^(file:\/{3})|(qrc:\/{2})|(http:\/{2})/,""))
                                                    : decodeURIComponent(cleanPath.replace(/^(file:\/{2})|(qrc:\/{2})|(http:\/{2})/,""));

            destFolder.text = cleanPath;
        }
    }

    Component.onCompleted: {
        var cleanPath = folderDialog.currentFolder.toString();
        cleanPath = (Qt.platform.os==="windows") ? decodeURIComponent(cleanPath.replace(/^(file:\/{3})|(qrc:\/{2})|(http:\/{2})/,""))
                                                : decodeURIComponent(cleanPath.replace(/^(file:\/{2})|(qrc:\/{2})|(http:\/{2})/,""));

        destFolder.text = cleanPath;
    }

    MessageDialog
    {
        id: resultDialog
        title: "DONE!!!"

        text: "Converted files: " + fileOkCount + "\n" + ((fileCount - fileOkCount) ? "Not converted files: " + (fileCount - fileOkCount) : "")
    }

    MessageDialog
    {
        id: errorMsg

        title: qsTr("Error")
    }

    Item
    {
        id : rWait

        anchors.fill: parent
        visible: main.wait

        Rectangle{
            anchors.fill: parent
            opacity: 0.5

            color: "grey"
        }

        ProgressBar
        {
            id: prBar
            height: parent.height/10*1
            width: parent.width*0.5

            visible: rWait.visible

            anchors.centerIn: parent
            value: 0
        }

//        BusyIndicator
//        {
//            id : bI
//            visible: rWait.visible

//            width:  parent.width/10
//            height: width
//            anchors.centerIn: parent
//        }

        MouseArea
        {
            id : mWait
            anchors.fill: parent
        }
    }

    ListModel
    {
        id: sourceModel
    }

    Connections
    {
        target: _core

        function onSgResamplingFinished(fileName)
        {
            for(var i=0; i < sourceModel.rowCount(); i++)
            {
                if(fileName === sourceModel.get(i).fileName)
                {
                    sourceModel.remove(i);

                    fileOkCount++;
                    prBar.value++;
                }
            }
        }

        function onSgOperationFinished()
        {
            resultDialog.open();
            main.wait = false;
            afterConvert = true;

        }

        function onSgError(description)
        {
            errorMsg.text = description;
            errorMsg.open();
        }
    }
}
