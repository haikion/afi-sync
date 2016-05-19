import QtQuick 2.0
import QtQuick.Controls 1.4


Rectangle {

    property string labelText
    property string fieldText
    property string unit: "KB/s"
    signal fieldChanged()
    //Work-a-round ... Property binding gets old value.
    function getFieldText() {
        return tf.text
    }

    CheckBox {
        id: cb
        checked: fieldText != "0"
        anchors.verticalCenter: parent.verticalCenter
        onCheckedChanged: {
            if (!checked) {
                tf.text = "0"
                fieldChanged()
            }
        }
    }

    Text {
        id: label
        text: labelText
        font.pixelSize: labelFont
        anchors.left: cb.right
        anchors.leftMargin: 3
        anchors.verticalCenter: parent.verticalCenter
    }

    TextField {
        property bool init: true
        //Wait at least one second before apply
        property Timer spamFilter: Timer {
            interval: 1000
            onTriggered: parent.parent.fieldChanged()
        }
        id: tf
        validator: IntValidator {bottom: 0}
        enabled: cb.checked
        height: parent.height
        x: 130
        anchors.verticalCenter: parent.verticalCenter
        onTextChanged: {
            if (init)
            {
                return;
            }
            spamFilter.restart()
            //parent.fieldChanged()
        }
        Component.onCompleted: {
            text = fieldText
            init = false
            //label.verticalCenter = verticalCenter
        }
    }

    Text {
        text: unit
        font.pixelSize: labelFont
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: tf.right
        anchors.leftMargin: 3
    }
}
