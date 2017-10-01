//Draws tree representation of mod-repository relations.
import QtQuick 2.3
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.1
import org.AFISync 0.1
import "." //Enables Global.qml

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

    onCurrentIndexChanged: console.log("current index: " + currentIndex
                                       + " current row: " + currentIndex.row)

    style: TreeViewStyle {
        backgroundColor: defaultColor
    }

    Menu {
        id: contextMenu
        property var index
        function display(idx)
        {
            index = idx;
            popup()
        }

        MenuItem {
            text: "Recheck"
            onTriggered: {
                console.log(contextMenu.index)
                if (!TreeModel.ticked(contextMenu.index))
                {
                    checkDialog.open()
                    return
                }

                TreeModel.processCompletion(contextMenu.index)
            }
        }
    }

    MessageDialog {
        id: checkDialog
        text: "Needs to be actived before rechecking."
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
        delegate: Rectangle {
            color: "transparent"
            CheckBox {
                id: cb
                checked: styleData.value !== "false"
                enabled: styleData.value !== "disabled"
                anchors.verticalCenter: parent.verticalCenter
                onClicked:  {
                    var idx = styleData.index;
                    TreeModel.checkboxClicked(idx)
                    repositoryList.enabled = true
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
    }

    MouseArea {
        id: contextArea
        anchors.fill: parent
        acceptedButtons: Qt.RightButton

        onClicked: {
            mouse.accepted = false;
            var index = repositoryList.indexAt(mouse.x, mouse.y)
            if (!index.valid)
            {
                return;
            }

            console.log("Index " + index)
            if (mouse.button === Qt.RightButton)
            {
                contextMenu.display(index);
            }
            else
            {
                repositoryList.updateCheckboxes()
                if (repositoryList.isExpanded(index)) {
                    repositoryList.collapse(index)
                } else {
                    repositoryList.expand(index)
                }
            }
        }
    }

    TableViewColumn {
        id: nameColumn
        title: "Name"
        role: "name"
        resizable : false
        width: repositoryList.width - checkColumn.width - statusColumn.width - progressColumn.width - startColumn.width -
               joinColumn.width - fileSizeColumn.width - 15;
        delegate: Rectangle {
            color: defaultColor

            Text {
                text: styleData.value
                anchors.verticalCenter: parent.verticalCenter
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton

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
        width: 110
    }

    TableViewColumn {
        id: progressColumn
        title: "ETA"
        role: "progress"
        width: 60
        movable: false
        resizable: false
    }

    TableViewColumn {
        id: fileSizeColumn
        title: "Size"
        role: "fileSize"
        width: 60
        movable: false
        resizable: false
    }

    Component {
        id: startButtonDelegate
        TreeViewButton {
            text: "Start Game"
            onClicked: {
                Global.armaStarted = true
                TreeModel.launch(styleData.index)
            }
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
            onClicked: {
                Global.armaStarted = true
                TreeModel.join(styleData.index)
            }
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
