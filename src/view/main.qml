import QtQuick 2.3
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtQuick.Window 2.2
import org.AFISync 0.1

//TODO: Max, min size, column resize when small.

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1100
    height: 700
    color: "lightslategray"
    title: "AFISync v0.25"

    Component.onDestruction:
    {
        console.log("Closing mainWindow")
    }
    onClosing:
    {
        console.log("onClosing event");
        Qt.quit()
        //TreeModel.killItWithFire();
    }

    MainView {
        id: mainView
        width: mainWindow.width - settings.width
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
    }

    Rectangle {
        id: expandButton
        width: 25
        height: 60
        color: "LightGrey"
        anchors.right: settings.left
        anchors.top: mainWindow.top
        anchors.topMargin: 20
        //anchors.rightMargin: 10
        radius: 2

        Text {
            id: text1
            text: qsTr("Settings")
            clip: true
            rotation: -90
            font.pixelSize: 12
            anchors.centerIn: parent
        }
        MouseArea {
            property bool toggle: false

            anchors.fill: parent
            onClicked: {
                toggle = !toggle
                if (toggle) {
                    settings.visible = true
                    settings.width = 400
                } else {
                    settings.visible = false
                    settings.width = 0
                }
            }
        }
    }
    Rectangle {
        id: settings
        anchors.left: mainView.right
        anchors.right: mainWindow.right
        anchors.top: mainWindow.top
        height: mainWindow.height
        //anchors.bottom: mainWindow.bottom
        visible: false
        color: "lightslategray"
        //z: 10
        Settings {
            anchors.fill: parent
            mainView: mainView
            onApply: {
                mainView.syncViewObj.updateCheckBoxes()
            }
        }
        Text {
            text: "by Hoxzer"
            anchors.right:  parent.right
            anchors.bottom: parent.bottom
            //width: 100
        }
    }
}
