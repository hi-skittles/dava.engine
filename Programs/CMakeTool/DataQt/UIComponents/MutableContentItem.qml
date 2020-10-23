import QtQuick 2.6
import QtQuick.Controls 1.5
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3

GroupBox {
    id: mutableContentItem;
    signal dataUpdated();
    property var localConfiguration;
    Layout.fillWidth: true;
    function processConfiguration(configuration) {
        localConfiguration = configuration;
        var arrayPlatforms = localConfiguration["platforms"];
        if(arrayPlatforms && Array.isArray(arrayPlatforms)) {
            for(var i = 0, length = arrayPlatforms.length; i < length; ++i) {
                listModel_platforms.append(arrayPlatforms[i]);
            }
        }
        var arrayGlobalOptions = localConfiguration["global options"];
        if(arrayGlobalOptions && Array.isArray(arrayGlobalOptions)) {
            for(var i = 0, length = arrayGlobalOptions.length; i < length; ++i) {
                listModel_globalOptions.append(arrayGlobalOptions[i]);
            }
        }
        localConfiguration["currentOptions"] = [];
        localConfiguration["checkedGlobalOptions"] = [];
    }
    function loadState(state) {
        localConfiguration["currentPlatform"] = state.platform;
        localConfiguration["currentOptions"] = state.currentOptions;
        localConfiguration["checkedGlobalOptions"] = state.checkedGlobalOptions;
        impl.configUpdated();
    }

    function saveState() {
        var state = {};
        state.platform = localConfiguration["currentPlatform"];
        state.currentOptions = localConfiguration["currentOptions"];
        state.checkedGlobalOptions = localConfiguration["checkedGlobalOptions"];
        //make a deep copy
        return JSON.parse(JSON.stringify(state));
    }

    Item {
        id: impl //item to incapsulate private functions
        signal configUpdated();
        function processDataChanged(checked, value, key) {
            var options = localConfiguration[key];
            var valueStr = JSON.stringify(value);
            var itemIndex = -1;
            for(var i = 0, count = options.length; i < count; i++) {
                if(JSON.stringify(options[i]) === valueStr) {
                    itemIndex = i;
                    break;
                }
            }
            if(checked) {
                if(itemIndex === -1) {
                    options.push(value)
                }
            } else {
                if(itemIndex !== -1) {
                    options.splice(itemIndex, 1)
                }
            }

            dataUpdated();
        }

    }

   
    ColumnLayout {
        id: columnLayout
        spacing: 10
        Layout.fillWidth: true;
        RowLayout {
            id: rowLayout
            anchors.fill: parent
            ListModel {
                id: listModel_platforms
            }
            ListModel {
                id: listModel_localOptions
            }

            ColumnLayout {
                id: columnLayout_platforms
                anchors.top: parent.top
                width: rowLayout.width / 2
                Label {
                    id: label_platforms
                    text: qsTr("Platforms")
                }

                Column {
                    id: column_platforms
                    spacing: 10
                    ExclusiveGroup {
                        id: exclusiveGroup_platforms
                    }
                    Repeater {
                        model: listModel_platforms
                        delegate: RadioButton {
                            text: model.name
                            exclusiveGroup: exclusiveGroup_platforms
                            Connections {
                                target: impl
                                onConfigUpdated:  {
                                    if(index === localConfiguration["currentPlatform"]) {
                                         checked = true;
                                     }
                                }
                            }

                            onCheckedChanged: {
                                if(checked && localConfiguration) {
                                    localConfiguration["currentPlatform"] = index;
                                    listModel_localOptions.clear();
                                    localConfiguration["currentOptions"] = [];
                                    var localObject = localConfiguration["platforms"][index];
                                    var options = JSON.parse(JSON.stringify(localObject["options"])); //make a copy
                                    if(options && Array.isArray(options)) {
                                        for(var i = 0, length = options.length; i < length; ++i) {
                                            options[i]["parentIndex"] = index
                                            listModel_localOptions.append(options[i]);
                                            impl.configUpdated();
                                        }
                                    }
                                    dataUpdated();
                                }
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                id: columnLayout_localOptions
                anchors.top: parent.top
                width: rowLayout.width / 2
                visible: listModel_localOptions.count !== 0
                Label {
                    id: label_options
                    text: qsTr("Options")
                }

                Column {
                    id: column_localOptions
                    spacing: 10
                    Repeater {
                        model: listModel_localOptions
                        delegate: loaderComponent
                    }
                    Component {
                        id: loaderComponent
                        Loader {
                            id: loaderDelegate
                            sourceComponent: type == "checkbox" ? checkboxDelegate : radioDelegate;
                            property variant modelData: listModel_localOptions.get(model.index);
                            property int index : model.index;
                            function createObj() {
                                return {"index": modelData["substring number"] - 1, "value": modelData.value};
                            }
                            Connections {
                                target: impl
                                onConfigUpdated:  {
                                    var obj = localConfiguration["currentOptions"];
                                    var currentObj = loaderDelegate.createObj();
                                    var currentObjStr = JSON.stringify(currentObj);
                                    var found = false;
                                    for(var i = 0, length = obj.length; i < length && !found; ++i) {
                                        if(JSON.stringify(obj[i]) === currentObjStr) {
                                            found = true;
                                        }
                                    }
                                    item.checked = found;
                                }
                            }
                        }
                    }
                    ExclusiveGroup {
                        id: exclusiveGroup_localOptions
                    }

                    Component {
                        id: radioDelegate
                        RadioButton {
                            text: modelData ? modelData.name : ""
                            onCheckedChanged: {
                                impl.processDataChanged(checked, createObj(), "currentOptions");
                            }
                            exclusiveGroup: exclusiveGroup_localOptions

                        }
                    }
                    Component {
                        id: checkboxDelegate
                        CheckBox {
                            text: modelData ? modelData.name : ""
                            onCheckedChanged: {
                                impl.processDataChanged(checked, createObj(), "currentOptions");
                            }
                        }
                    }
                }
            }
        }
        ColumnLayout {
            id: columnLayout_globalOptions
            width: rowLayout.width
            Label {
                id: label_globalOptions
                text: qsTr("Global options")
            }

            ListModel {
                id: listModel_globalOptions
            }

            Flow {
                id: flow_globalOptions
                width: parent.width
                spacing: 10
                Repeater {
                    model: listModel_globalOptions
                    CheckBox {
                        text: model.name
                        onCheckedChanged: {
                            impl.processDataChanged(checked, {"value": model.value}, "checkedGlobalOptions");

                        }
                        Connections {
                            target: impl
                            onConfigUpdated:  {
                                var obj = localConfiguration["checkedGlobalOptions"];
                                if(obj === undefined) {
                                    return;
                                }

                                var currentObj = {"value": model.value};
                                var currentObjStr = JSON.stringify(currentObj);
                                var found = false;
                                for(var i = 0, objLength = obj.length; i < objLength && !found; ++i) {
                                    if(JSON.stringify(obj[i]) === currentObjStr) {
                                        found = true;
                                    }
                                }
                                checked = found;
                            }
                        }
                    }
                }
            }
        }
    }
}
