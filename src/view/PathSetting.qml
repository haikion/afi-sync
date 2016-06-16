import QtQuick 2.3
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

Column {
    property variant fileDialog
    property string label

    property var getter
    signal setter(string newValue)
    signal resetter

    spacing: 1
    width: parent.width - fieldSpacing
    anchors.left: parent.left
    anchors.leftMargin: fieldSpacing/2

    Text {
        text: label
        font.pixelSize: 12
    }

    TextField {
        id: pf
        onTextChanged: setter(text)
        width: parent.width
        height: defaultHeight
        text: getter()
    }
    Row {
        height: defaultHeight
        spacing: buttonSpacing
        Button {
            text: "Browse"
            height: parent.height
            onClicked: {
                fileDialog.caller = pf
                fileDialog.visible = true
            }
        }

        Button {
            text: "Reset Default"
            height: parent.height
            onClicked: {
                resetter()
                pf.text = getter()
            }
        }
    }
}
