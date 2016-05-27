import QtQuick 2.3
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2
import org.AFISync 0.1

Item {
    id: afiSyncWindow
    visible: true
    property bool initializing: true
    property variant syncViewObj: syncView
    property string downloadStr
    property string uploadStr

    Component.onCompleted: {
        //syncViewObj = syncViewComponent.createObject(afiSyncWindow);
        //createSync()
        console.log("loaded " + parent.enabled + " " + parent.visible + " " + parent.width + " " + parent.height)
        parent.visible = false;
        parent.visible = true;
        speedTimer.running = true;
    }

    MessageDialog {
        id: arma3RunningDlg
        title: "Warning"
        text: "ArmA 3 is running during download. Please close ArmA 3."
    }

    Timer {
        id: speedTimer
        interval: 1
        running: false
        repeat: true
        onTriggered: {
            downloadStr = TreeModel.getDownload()
            uploadStr = TreeModel.getUpload()
            if (parseInt(downloadStr) > 0 && ProcessMonitor.arma3Running())
                arma3RunningDlg.visible = true
        }
    }

    SyncView {
        id: syncView
        anchors.top: titleRectangle.bottom
        anchors.bottom: footerRectangle.top
    }

    Rectangle {
        id: titleRectangle
        y: 0
        height: 70
        color: "lightslategray"
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        Image {
            id: name
            source: "qrc:/afisync_header.png"
            Component.onCompleted: {
                var origW = width
                width = Qt.binding(function () {return Math.min(origW, parent.width)})
            }
        }
    }

    Rectangle {
        id: footerRectangle
        height: 19
        color: titleRectangle.color
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0

        Rectangle {
            id: rectangle1
            width: 328
            color: parent.color
            anchors.horizontalCenterOffset: 0
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0

            Text {
                id: downloadText
                width: 93
                text: "Download:"
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.left: parent.left
                anchors.leftMargin: 4
                font.bold: true
                font.pixelSize: 15
            }

            Text {
                //Causes memory leak
                //Component.onCompleted: {
                //    text = Qt.binding(function () {return TreeModel.download})
                //}

                id: downloadAmountText
                y: 0
                width: 100
                text: downloadStr
                anchors.left: downloadText.right
                anchors.leftMargin: 0
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                anchors.topMargin: 0
                font.bold: true
                anchors.top: parent.top
                font.pixelSize: 15
            }

            Text {
                id: uploadText
                y: 0
                width: 68
                text: "Upload:"
                anchors.left: downloadAmountText.right
                anchors.leftMargin: 10
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                anchors.topMargin: 0
                font.bold: true
                anchors.top: parent.top
                font.pixelSize: 15
            }

            Text {
                //Memory leak
                //Component.onCompleted: {
                //    text = Qt.binding(function () {return TreeModel.upload})
                //}

                id: uploadAmountText
                y: 0
                text: uploadStr
                anchors.right: parent.right
                anchors.rightMargin: 0
                anchors.left: uploadText.right
                anchors.leftMargin: 0
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                anchors.topMargin: 0
                font.bold: true
                anchors.top: parent.top
                font.pixelSize: 15
            }



        }
        anchors.rightMargin: 0
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.leftMargin: 0
    }
}
