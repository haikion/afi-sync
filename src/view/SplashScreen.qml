import QtQuick 2.3
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtQuick.Window 2.2

Window {
    flags: Qt.SplashScreen
    width: 459
    height: 71 + 40
    visible: true
    color: "lightslategray"

    Image {
        id: image
        source: "qrc:/afisync_header.png"
        //anchors.fill: parent
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
        mainComponent.createObject()
        visible = false
    }
}
