//Draws tree representation of mod-repository relations.
import QtQuick 2.3
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import org.AFISync 0.1

TreeView {
    property string defaultColor: "LightGrey"
    id: repositoryList
    anchors.left: parent.left
    anchors.right: parent.right
    selectionMode: SelectionMode.NoSelection
    headerVisible: true
    model: TreeModel
    horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
    signal updateCheckboxes
    function updateCBs() {
        updateCheckboxes()
    }

    style: TreeViewStyle {
        backgroundColor: defaultColor
    }

    rowDelegate: Rectangle {
        height: 30
        color: defaultColor
    }

    TableViewColumn {
        id: checkColumn
        title: "Activate"
        role: "check"
        resizable: false
        width: 55
        delegate: CheckBox {
            id: cb
            checked: styleData.value !== "false"
            enabled: styleData.value !== "disabled"
            onClicked:  {
                var idx = styleData.index;
                TreeModel.checkboxClicked(idx)
                repositoryList.enabled = true
                console.log("Checked=" + checked + " styleData.value=" + styleData.value)
            }
            //Does not update otherwise.. qml kiddings me /__\
            Connections {
                target: repositoryList
                onUpdateCheckboxes: {
                    cb.checked = styleData.value !== "false"
                }
            }
        }
    }

    TableViewColumn {
        title: "Name"
        role: "name"
        resizable : false
        width: repositoryList.width - checkColumn.width - statusColumn.width - progressColumn.width - startColumn.width -
               joinColumn.width - 15;
        delegate: Rectangle {
            color: defaultColor

            Text {
                text: styleData.value
                anchors.verticalCenter: parent.verticalCenter
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    repositoryList.updateCheckboxes()
                    if (repositoryList.isExpanded(styleData.index)) {
                        repositoryList.collapse(styleData.index)
                    } else {
                        repositoryList.expand(styleData.index)
                    }
                }
            }
        }
    }

    TableViewColumn {
        id: statusColumn
        title: "Status"
        role: "status"
        resizable : false
        movable: false
        width: 180
    }

    TableViewColumn {
        id: progressColumn
        title: "ETA"
        role: "progress"
        width: 60
        movable: false
        resizable: false
    }

    Component {
        id: startButtonDelegate
        TreeViewButton {
            text: "Start Game"
            onClicked: TreeModel.launch(styleData.index)
            color: defaultColor
        }
    }

    TableViewColumn {
        id: startColumn
        role: "start"
        resizable : false
        movable: false
        width: 100
        delegate: startButtonDelegate
    }

    Component {
        id: joinButtonDelegate
        TreeViewButton {
            text: "Join Server"
            onClicked: TreeModel.join(styleData.index)
            color: defaultColor
        }
    }

    TableViewColumn {
        id: joinColumn
        role: "join"
        resizable : false
        movable: false
        width: startColumn.width
        delegate: joinButtonDelegate
    }
}
