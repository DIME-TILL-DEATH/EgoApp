import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs


MenuBar{
    id: mainMenu

    Menu{
        title: qsTr("File")

        MenuSeparator{}
        MenuItem{
            text: qsTr("Quit")
            onTriggered: {Qt.quit()}
        }
    }

    Menu{
        title: qsTr("Interface")

        MenuItem{
            text: qsTr("Show hotkeys")

            onTriggered: _hotKeysDialog.open()
        }

        Menu{
            id: languageMenu
            title: qsTr("Language")

             ActionGroup{
                 id: languageGroup
             }

             Action{
                 id: menuEn

                 text: "English"
                 checkable: true
                 checked: UiCore.appLanguageCode === "en"
                 ActionGroup.group: languageGroup

                 onTriggered: UiCore.setLanguage("en");
             }
             Action{
                 id: menuRu

                 text: "Русский"
                 checkable: true
                 checked: UiCore.appLanguageCode === "ru"
                 ActionGroup.group: languageGroup

                 onTriggered: UiCore.setLanguage("ru");
             }
        }
    }

    Menu{
        title: qsTr("Tools")


        MenuItem{
            text: qsTr("Legacy WAV convertor")

            onTriggered:
            {
                UiCore.runWavConvertor();
            }
        }
    }

    Menu{
        title: qsTr("Help")

        MenuItem{
            text: qsTr("AMT web site")

            onTriggered: {
                if(Qt.locale().name === "ru_RU")
                    Qt.openUrlExternally("http://www.amtelectronics.ru/")
                else
                    Qt.openUrlExternally("https://media.amtelectronics.com/")
            }
        }

        MenuItem{
            text: qsTr("EgoGig web page")

            onTriggered: {
                if(Qt.locale().name === "ru_RU")
                    Qt.openUrlExternally("https://media.amtelectronics.ru/amt-egogig-eg-4/")
                else
                    Qt.openUrlExternally("https://media.amtelectronics.com/amt-egogig-eg-4/")
            }
        }

        MenuItem{
            id: menuDeviceManual

            property string strManualBaseName: "EgoGIG_manual"

            text: qsTr("Device manual")

            onTriggered: {
                UiCore.openManualExternally(strManualBaseName)
            }
        }
        MenuSeparator{}
        MenuItem{
            text: qsTr("About...")
            onTriggered: aboutDialog.open()
        }
    }

    MessageDialog{
        id: _hotKeysDialog

        title: qsTr("Hotkeys")

        text: qsTr("Add song: ") + AppGlobals.addSongShortcut + "\n" +
              qsTr("Delete song: ") + AppGlobals.deleteSongShortcut + "\n" +
              qsTr("Set track1 link: ") + AppGlobals.setLink1Shortcut + "\n" +
              qsTr("Set track2 link: ") + AppGlobals.setLink2Shortcut + "\n" +
              qsTr("Toggle play next: ") + AppGlobals.togglePlayNextShortcut + "\n" +
              qsTr("Play EGO song: ") + AppGlobals.playEgoShortcut + "\n" +
              qsTr("Play WAV file: ") + AppGlobals.playWavShortcut
    }

    MessageDialog{
        id: aboutDialog

        title: qsTr("About...")
        text: qsTr("AMT EgoGIG")
        informativeText: qsTr("Desktop application") + "\n" +
              qsTr("Version: ") + Qt.application.version + "\n"
              + qsTr("(c) 2025")

    }
}
