import QtQuick 2.3
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import org.AFISync 0.1
import "."

Rectangle {
    property string text
    signal clicked()

    Component {
        id: button

        Button {
            property string value: parent.parent.styleData.value

            text: parent.text
            anchors.centerIn: parent
            width: parent.width - 10
            height: parent.height - 5
            //TODO: return bool through model
            enabled: value === "Join" || value === "Start"  || Global.buttonsEnabled
            onClicked: parent.clicked()
        }
    }

    property variant buttonVar
    Component.onCompleted: {
        if (TreeModel.isRepository(styleData.index)) {
            console.log("Create")
            buttonVar = button.createObject(this)
        }
    }
    Component.onDestruction: {
        if (typeof buttonVar != "undefined" && buttonVar !== null)
        {
            console.log("Destroy")
            button.destroy()
        }
    }
}
