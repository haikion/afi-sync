import QtQuick 2.3
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import org.AFISync 0.1

Window {
    flags: Qt.SplashScreen
    width: 459
    height: 71 + 40
    visible: true
    color: "lightslategray"

    Image {
        id: image
        source: "file:///" + applicationDirPath + "/afisync_header.png"
    }

    Text {
        anchors.top: image.bottom
        text: "Loading..."
        font.pixelSize: 20
        font.bold: true
        color: "blue"
    }

    Component {
        id: mainComponent
        MainWindow {
        }
    }

    Component.onCompleted: {
        console.log("loading width: " + image.width + " heigth: " + image.height)

        if (ProcessMonitor.afiSyncRunning()) {
            visible = false
            afiSyncRunningDlg.visible = true
        }
        else if (ProcessMonitor.arma3Running()) {
            visible = false
            arma3RunningDlg.visible = true
        }
        else {
            mainComponent.createObject()
            visible = false
        }
    }

    MessageDialog {
        id: arma3RunningDlg
        title: "Launch Aborted"
        text: "Arma 3 or its launcher is running. Please close it before running AFISync."
        onAccepted: {
            Qt.quit()
        }
    }

    MessageDialog {
        id: afiSyncRunningDlg
        title: "Launch Aborted"
        text: "AFISync is already running."
        onAccepted: {
            Qt.quit()
        }
    }
}
