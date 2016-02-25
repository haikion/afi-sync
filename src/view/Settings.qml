import QtQuick 2.3
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtQuick 2.2
import QtQuick.Dialogs 1.0
import org.AFISync 0.1


Column {
    property MainView mainView;
    property int labelFont: 12
    property string defaultColor: "DarkSeaGreen"
    property int fieldSpacing: 10
    property int buttonSpacing: 3
    property int defaultHeight: 20
    signal apply()

    anchors.margins: 10
    spacing: 20

    FileDialog {
        property variant caller;

        id: fileDialog
        title: "Please Choose Directory"
        selectFolder: true
        onAccepted: {
            var path = fileUrls.toString()
            path = path.replace(/^(file:\/{3})/,"");
            // unescape html codes like '%23' for '#'
            path = path.replace(/\//g, "\\")
            var cleanPath = decodeURIComponent(path);
            console.log("You chose: " + path)
            caller.text = cleanPath
        }
        onRejected: {
            console.log("Canceled")
        }
        onVisibilityChanged: {
            folder = "file:///" + caller.text
            console.log("Folder: " + Qt.resolvedUrl(caller.text))
        }
    }

    Rectangle {
        color: "transparent"
        width: parent.width
        border.width: 1
        height: title11.height + alp.height + be.height + 20

        SettingsTitle {
            id: title11
            text: "General"
            font.pixelSize: labelFont
        }

        Column {
            anchors.top: title11.bottom
            anchors.topMargin: 5
            width: parent.width - fieldSpacing
            anchors.left: parent.left
            anchors.leftMargin: fieldSpacing/2
            id: alp
            spacing: 1

            Text {
                text: qsTr("Additional Launch Parameters")
                font.pixelSize: labelFont
            }

            TextField {
                text: SettingsModel.launchParameters();
                onTextChanged: SettingsModel.setLaunchParameters(text);
                width: parent.width
                height: defaultHeight
            }
        }

        Row {
            anchors.top: alp.bottom
            anchors.topMargin: 3
            spacing: 4
            anchors.left: parent.left
            anchors.leftMargin: fieldSpacing/2
            id: be

            CheckBox {
                checked: SettingsModel.battlEyeEnabled();
                onCheckedChanged: SettingsModel.setBattlEyeEnabled(checked);
            }
            Text {
                text: qsTr("BattlEye  ")
                font.pixelSize: labelFont
            }
        }
    }

    Rectangle {
        color: "transparent"
        width: parent.width
        border.width: 1
        height: title10.height + 4*modDownloadPath.height + 20

        SettingsTitle {
            id: title10
            text: "Paths"
            font.pixelSize: labelFont
        }

        Column {
            spacing: 1
            width: parent.width - fieldSpacing
            anchors.top: title10.bottom
            anchors.topMargin: 5
            id: modDownloadPath
            anchors.left: parent.left
            anchors.leftMargin: 5

            Text {
                text: qsTr("Mod Download (Apply Required)")
                font.pixelSize: labelFont
            }

            TextField {
                id: pathField1

                text: SettingsModel.modDownloadPath();
                //onTextChanged: SettingsModel.setModDownloadPath(text);
                width: parent.width
                height: defaultHeight
            }
            Row {
                spacing: buttonSpacing
                height: defaultHeight
                Button {
                    text: "Browse"
                    height: parent.height
                    onClicked: {
                        fileDialog.caller = pathField1
                        fileDialog.visible = true
                    }
                }

                Button {
                    text: "Reset Default"
                    height: parent.height
                    onClicked: {
                        SettingsModel.resetModDownloadPath();
                        pathField1.text = SettingsModel.modDownloadPath();
                    }
                }
                Button {
                    text: "Apply"
                    height: parent.height
                    onClicked: {
                        var oldText = text
                        text = "Loading..."
                        enabled = false
                        SettingsModel.setModDownloadPath(pathField1.text)
                        apply()
                        enabled = true
                        text = oldText
                    }
                }
            }
        }

        PathSetting {
            anchors.top: modDownloadPath.bottom
            label: "Arma 3"
            fileDialog: fileDialog
            onSetter: SettingsModel.setArma3Path(newValue)
            getter: function() { return SettingsModel.arma3Path()}
            onResetter: SettingsModel.resetArma3Path()
            id: a3Path
        }

        PathSetting {
            anchors.top: a3Path.bottom
            label: "TeamSpeak 3"
            fileDialog: fileDialog
            onSetter: SettingsModel.setTeamSpeak3Path(newValue)
            getter: function() { return SettingsModel.teamSpeak3Path() }
            onResetter: SettingsModel.resetTeamSpeak3Path()
            //labelFont: labelFont
            id: ts3Path
        }

        PathSetting {
            anchors.top: ts3Path.bottom
            label: "Steam"
            fileDialog: fileDialog
            onSetter: SettingsModel.setSteamPath(newValue)
            getter: function() { return SettingsModel.steamPath() }
            onResetter: SettingsModel.resetSteamPath()
        }

    }

    Rectangle {
        color: "transparent"
        width: parent.width
        height: 4*defaultHeight
        border.width: 1
        id: bandwidthRect

        Text {
            id: title
            text: "Bandwidth Limits"
            font.pixelSize: labelFont
            anchors.top: parent.top
            anchors.topMargin: 3
            anchors.left: parent.left
            anchors.leftMargin: 3
        }


        BandwidthRow {
            id: uploadRow
            anchors.top: title.bottom
            anchors.topMargin: 3
            anchors.left: parent.left
            anchors.leftMargin: 3
            height: defaultHeight
            labelText: "Upload:"
            fieldText: SettingsModel.maxUpload()
            onFieldChanged: {
                console.log("upload set: " + getFieldText())
                SettingsModel.setMaxUpload(getFieldText())
            }
        }

        BandwidthRow {
            anchors.top: uploadRow.bottom
            anchors.topMargin: 3
            anchors.left: parent.left
            anchors.leftMargin: 3
            height: defaultHeight
            labelText: "Download:"
            fieldText: SettingsModel.maxDownload()
            onFieldChanged: {
                console.log("Download set: " + getFieldText())
                SettingsModel.setMaxDownload(getFieldText())
            }
        }
    }

    Rectangle {
        color: "transparent"
        width: parent.width
        border.width: 1
        height: troublesColumn.height + title2.height + 15

        SettingsTitle {
            id: title2
            text: "Troubleshooting"
            font.pixelSize: labelFont
        }

        Column {
            anchors.top: title2.bottom
            anchors.topMargin: 5
            width: parent.width - 10
            anchors.horizontalCenter: parent.horizontalCenter
            id: troublesColumn

            Button {
                text: "Manual Installation and Extra File Deletion"
                onClicked: {
                    var cache = text;
                    text = "Loading..."
                    enabled = false
                    TreeModel.processCompletion()
                    enabled = true
                    text = cache
                }
                height: defaultHeight
                width: parent.width
            }
            Button {
                text: "Reset Sync"
                onClicked: {
                    var cache = text;
                    text = "Loading..."
                    enabled = false
                    TreeModel.resetSync()
                    apply()
                    enabled = true
                    text = cache
                }
                height: defaultHeight
                width: parent.width
            }
        }
    }
}
