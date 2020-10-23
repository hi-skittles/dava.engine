import QtQuick 2.6
import QtQuick.Controls 1.5
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import Cpp.Utils 1.0

//qml Dialog not working
// using Window instead 
Window {
    id: preferencesDialog
    visible: false
    modality: Qt.ApplicationModal
    flags: Qt.Tool
    title: "CMakeTool preferences dialog"
    property int margin: 10
    width: preferencesColumnLayout.implicitWidth + 2 * margin
    height: preferencesColumnLayout.implicitHeight + 2 * margin
    minimumWidth: preferencesColumnLayout.minimumWidth + 2 * margin
    minimumHeight: preferencesColumnLayout.minimumHeight + 2 * margin
    
    property var settings;
    
    SystemPalette {
        id: palette; 
    }
    function onAccepted() {   
        settings.buildToTheSourceFolder = radio_buildToSourceFolder.checked
        settings.customBuildFolder = rowLayout_customBuildFolder.path
    }
    onVisibleChanged: { 
        if(visible) {
            radio_buildToSourceFolder.checked = settings.buildToTheSourceFolder
            radio_buildToAnotherFolder.checked = !radio_buildToSourceFolder.checked
            rowLayout_customBuildFolder.path = settings.customBuildFolder
        }
    }
    //default palette not work in window
    Rectangle {
        anchors.fill: parent
        color: palette.window
        //margins not work in layout
        Rectangle {
            anchors.fill: parent
            anchors.margins: margin
            color: "transparent"
            ColumnLayout {
                id: preferencesColumnLayout
                anchors.fill: parent
                ExclusiveGroup {
                    id: exclusiveGroup_buildFolder
                }
                RadioButton {
                    id: radio_buildToSourceFolder
                    Layout.fillWidth: true;
                    text: "Build to the source folder: \"$SOURCE/_build\""
                    exclusiveGroup: exclusiveGroup_buildFolder
                }
                RadioButton {
                    id: radio_buildToAnotherFolder
                    Layout.fillWidth: true;
                    text: "Build to custom folder \"$CUSTOM_FOLDER/$PROJECT/_build\""
                    exclusiveGroup: exclusiveGroup_buildFolder
                }
                RowLayoutPath {
                    id: rowLayout_customBuildFolder
                    enabled: radio_buildToAnotherFolder.checked
                    Layout.fillWidth: true;
                    labelText: qsTr("Custom build folder")
                    dialogTitle: qsTr("Select custom build folder");
                    selectFolders: true;
                    inputComponent: TextField {
                        placeholderText: qsTr("path to custom build folder")
                    }
                }
                Item {
                    Layout.fillHeight: true
                }
                RowLayout {
                    Item {
                        Layout.fillWidth: true
                    }
                    Button {
                        text: qsTr("Ok");
                        onClicked: { 
                            preferencesDialog.onAccepted();
                            preferencesDialog.visible = false;
                        }
                    }
                    Button {
                        text: qsTr("Cancel");
                        onClicked: { 
                            preferencesDialog.visible = false;
                        }            
                    }
                }
            }
        }
    }
}