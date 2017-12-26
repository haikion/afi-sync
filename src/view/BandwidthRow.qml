import QtQuick 2.3
import QtQuick.Controls 1.4


Rectangle {

    property string labelText
    property string fieldText
    property string unit: "KB/s"
    property string defaultValue: ""
    property bool checked: cb.checked
    signal fieldChanged()
    //Work-a-round ... Property binding gets old value.
    function getFieldText() {
        return tf.text
    }
    function getEnabled() {
        return cb.checked
    }
    anchors.topMargin: 3
    anchors.left: parent.left
    anchors.leftMargin: 3

    CheckBox {
        id: cb
        checked: true
        anchors.verticalCenter: parent.verticalCenter
        onCheckedChanged: {
            fieldChanged()
        }

        Component.onCompleted:
        {
            checked = parent.checked
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
        }
        Component.onCompleted: {
            text = fieldText
            init = false
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
