import QtQuick 2.3
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import org.AFISync 0.1
import "."

Rectangle {
    id: rect
    property string text
    property variant buttonVar
    property bool isRepo: false
    signal clicked()
    property string value2: parent != null ? parent.styleData.value : "hidden"

    /*
        Created on every line but only visible on repository because:
        Note: For performance reasons, created delegates can be recycled across
        multiple table rows. This implies that when you make use of implicit properties
        such as styleData.row or model, these values can change after the delegate has been
        constructed. This means that you should not assume that content is fixed when
        Component.onCompleted is called, but instead rely on bindings to such properties."
    */
    Button {
        id: button

        property string value: parent.parent != null ? parent.parent.styleData.value : "disabled"

        visible: styleData.hasChildren //Only repositories have children
        text: parent.text
        anchors.centerIn: parent
        width: parent.width - 10
        height: parent.height - 5
        enabled: value === "Join" || value === "Start"  || Global.buttonsEnabled
        onClicked: parent.clicked()
    }
}
