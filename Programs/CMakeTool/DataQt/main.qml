import QtQuick 2.6
import QtQuick.Controls 1.5
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3
import Cpp.Utils 1.0
import Qt.labs.settings 1.0
import "UIComponents"
import "Scripts/commandLine.js" as JSTools

ApplicationWindow {
    id: applicationWindow
    visible: true
    width: 800
    height: 600
    property int historyVersion: 4
    property string davaFolderName: "dava.framework";
    objectName: "applicationWindow"
    minimumHeight: wrapper.Layout.minimumHeight + splitView.margins * 4 + wrapper.spacing * 4
    minimumWidth: wrapper.Layout.minimumWidth + splitView.anchors.margins * 2 + 50
    //on os x minimum height changes several times during program launch and changed values not linear
    PlatformHelper {
        id: platformHelper
    }
    function normalizeHeight() {
        if(applicationWindow.height < applicationWindow.minimumHeight) {
            applicationWindow.height = applicationWindow.minimumHeight
        }
    }
    function normalizeWidth() {
        if(applicationWindow.width < applicationWindow.minimumWidth) {
            applicationWindow.width = applicationWindow.minimumWidth
        }
    }
    Timer {
        id: geometryTimer
        repeat: false
        interval: 200
        onTriggered: {
            normalizeHeight();
            normalizeWidth();
        }
    }
    onMinimumHeightChanged: {
        if(platformHelper.CurrentPlatform() == PlatformHelper.Windows ) {
            normalizeHeight();
        } else {
            geometryTimer.restart();
        }
    }

    onMinimumWidthChanged: {
        if(platformHelper.CurrentPlatform() == PlatformHelper.Windows ) {
            normalizeWidth();
        } else {
            geometryTimer.restart();
        }    
    }
    toolBar: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                tooltip: qsTr("Preferences")
                iconSource: "qrc:///Icons/settings.png"
                onClicked: preferencesDialog.show();
            }
            ToolButton {
                tooltip: qsTr("Show help")
                iconSource: "qrc:///Icons/help.png"
                onClicked: help.Show();
            }
            Item {
                Layout.fillWidth: true
            }
        }
    }
    function processText(text) {
        return text.replace(/(\r\n|\r|\n)+$/g, "").replace(/(\r\n|\r|\n)+/g, displayHtmlFormat ? "<br>" : "\n");
    }
    property bool displayHtmlFormat: true
    Settings {
        id: settings
        property int mainWrapperWidth: 400
        property alias x: applicationWindow.x
        property alias y: applicationWindow.y
        property alias width: applicationWindow.width
        property alias height: applicationWindow.height
        
        property alias prefWidth: preferencesDialog.width
        property alias prefHeight: preferencesDialog.height
        property alias prefX: preferencesDialog.x
        property alias prefY: preferencesDialog.y
        
        property string historyStr;
        property var lastUsedSourceFolder;
        Component.onDestruction: {
            outputState = columnLayoutOutput.saveState();
            function compare(left, right) {
                return left.source.localeCompare(right.source);
            }
            history.sort(compare);
            historyStr = JSON.stringify(history)
            historyVersion = applicationWindow.historyVersion;
        }
        property int historyVersion: -1
        property bool buildToTheSourceFolder: true
        property string customBuildFolder;
        property var outputState;
    }
    property var history;
    function applyProjectSettings(buildSettings) {
        rowLayout_buildFolder.path = buildSettings.buildFolder;
        rowLayout_cmakeFolder.path = buildSettings.cmakePath;
        rowLayout_davaFolder.path = buildSettings.davaPath;
        textField_customOptions.text = buildSettings.customOptions
        mutableContent.loadState(buildSettings.state);
    }

    function loadHistory() {
        history = [];

        if(settings.historyVersion === historyVersion) {
            history = JSON.parse(settings.historyStr);
        }
        for(var i = history.length - 1; i >= 0; --i) {
            var source = history[i].source;
            if(!fileSystemHelper.IsDirExists(source)) {
                history.splice(i, 1);
            }
        }
        for(var i = 0, length = history.length; i < length; ++i) {
            var source = history[i].source;
            rowLayout_sourceFolder.item.addString(history[i].source)
        }
    }

    function onCurrentProjectChaged(index) {
        if(history && Array.isArray(history) && history.length > index) {
            var historyItem = history[index];
            applyProjectSettings(historyItem)
        }
    }

    function addProjectToHistory() {
        var found = false;
        var source = rowLayout_sourceFolder.path;
        settings.lastUsedSourceFolder = source;
        source = fileSystemHelper.NormalizePath(source);
        
        var newItem = {};
        newItem.source = source
        newItem.buildFolder = rowLayout_buildFolder.path
        newItem.cmakePath = rowLayout_cmakeFolder.path
        newItem.davaPath = rowLayout_davaFolder.path
        newItem.customOptions = textField_customOptions.text
        newItem.state = mutableContent.saveState();
        
        //now update current history, because we load fields from it.
        for(var i = 0, length = history.length; i < length && !found; ++i) {
            if(history[i].source === source) {
                found = true;
                history[i] = newItem;
            }
        }
        
        //add to combobox and to history
        if(!found) {
            history.push(newItem)
            rowLayout_sourceFolder.item.addString(source)
        }
    }

    title: qsTr("CMake tool")

    ProcessWrapper {
        id: processWrapper;
        Component.onDestruction: BlockingStopAllTasks();
    }

    ConfigStorage {
        id: configStorage
    }

    FileSystemHelper {
        id: fileSystemHelper;
    }

    Help {
        id: help;
    }
    
    PreferencesDialog {
        id: preferencesDialog
        visible: false
        settings: settings
    }

    property var configuration; //main JS object, contained in config file

    Timer {
        id: timer;
        interval: 10
        repeat: false
        onTriggered: updateOutputStringImpl();
    }

    function updateOutputString() {
        timer.start();
    }

    function updateOutputStringImpl() {
        if(configuration) {
            var sourcePath = fileSystemHelper.NormalizePath(rowLayout_sourceFolder.path)
            var buildPath = fileSystemHelper.NormalizePath(rowLayout_buildFolder.path)
            var cmakePath = fileSystemHelper.NormalizePath(rowLayout_cmakeFolder.path)
            var davaPath = fileSystemHelper.NormalizePath(rowLayout_davaFolder.path)
            var customOptions = textField_customOptions.text
            try {
                var outputText = JSTools.createOutput(configuration,
                                                      fileSystemHelper,
                                                      sourcePath,
                                                      buildPath,
                                                      cmakePath,
                                                      davaPath,
                                                      customOptions);
                columnLayoutOutput.outputComplete = true;
                columnLayoutOutput.outputText = outputText;
            } catch(errorText) {
                columnLayoutOutput.outputComplete = false;
                columnLayoutOutput.outputText = errorText.toString();
            }
        }
    }

    Component.onCompleted: {
        try {
            configuration = JSON.parse(configStorage.GetJSONTextFromConfigFile());
            mutableContent.processConfiguration(configuration);
            columnLayoutOutput.loadState(settings.outputState);
            loadHistory();
            var lastSource = settings.lastUsedSourceFolder;
            for(var i = 0, length = history.length; i < length; ++i) {
                if(history[i].source === lastSource) {
                    rowLayout_sourceFolder.item.currentIndex = i;
                    onCurrentProjectChaged(i)
                    break;
                }
            }
        }
        catch(error) {
            errorDialog.informativeText = error.message;
            errorDialog.critical = true;
            errorDialog.open();
        }
        applicationWindow.width = Math.max(applicationWindow.width, applicationWindow.minimumWidth)
        applicationWindow.height = Math.max(applicationWindow.height, applicationWindow.minimumHeight)
    }

    MessageDialog {
        id: errorDialog;
        text: qsTr("error occurred!")
        icon: StandardIcon.Warning
        property bool critical: false
        onVisibleChanged: {
            if(!visible && critical) {
                Qt.quit()
            }
        }
    }

    SplitView {
        id: splitView;
        anchors.fill: parent
        property int margins: 10
        anchors.margins: margins
        objectName: "splitView"

        Item {
            id: wrapperItem
            width: settings.mainWrapperWidth;
            Component.onDestruction: {
                settings.mainWrapperWidth = width
            }
            Layout.minimumWidth: wrapper.Layout.minimumWidth
            ColumnLayout {
                id: wrapper
                anchors.fill: parent
                anchors.rightMargin: splitView.margins
                RowLayoutPath {
                    id: rowLayout_sourceFolder
                    labelText: qsTr("Source folder");
                    dialogTitle: qsTr("select Source folder");
                    selectFolders: true;
                    inputComponent: ComboBoxBuilds {
                        onTextChanged: {
                            updateOutputString()
                            var found = false;
                            for(var i = 0, length = history.length; i < length && !found; ++i) {
                                if(history[i].source == text) {
                                    found = true;
                                    onCurrentProjectChaged(i);
                                }
                            }
                            if (!found) {
                                if (settings.buildToTheSourceFolder) {
                                    rowLayout_buildFolder.path = text + "/_build";
                                } else {
                                    var array = text.split(/[\\\/]+/g);
                                    if(array.length > 0) {
                                        var path = settings.customBuildFolder + "/" + array[array.length - 1] + "/_build";
                                        rowLayout_buildFolder.path = fileSystemHelper.NormalizePath(path);
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayoutPath {
                    id: rowLayout_buildFolder
                    labelText: qsTr("Build folder")
                    dialogTitle: qsTr("select build folder");
                    selectFolders: true;
                    inputComponent: TextField {
                        id: textField_buildFolder
                        placeholderText: qsTr("path to source folder")
                        onTextChanged: {
                            updateOutputString();
                        }
                    }
                }

                RowLayoutPath {
                    id: rowLayout_davaFolder
                    labelText: qsTr("DAVA folder")
                    dialogTitle: qsTr("select DAVA folder");
                    selectFolders: true;
                    inputComponent: TextField {
                        id: textField_davaFolder
                        placeholderText: qsTr("path to dava folder")
                        onTextChanged: {
                            updateOutputString();
                        }
                    }
                    Connections {
                        target: rowLayout_sourceFolder
                        onPathChanged: {
                            var path = rowLayout_sourceFolder.path;
                            var index = path.indexOf(davaFolderName);
                            if(index !== -1) {
                                path = path.substring(0, index + davaFolderName.length);
                                rowLayout_davaFolder.path = path;
                            }
                        }
                    }
                }

                RowLayoutPath {
                    id: rowLayout_cmakeFolder
                    labelText: qsTr("CMake folder")
                    dialogTitle: qsTr("select CMake executable");
                    selectFolders: false;
                    inputComponent: TextField {
                        id: textField_cmakeFolder
                        placeholderText: qsTr("path to CMake folder")
                        onTextChanged: {
                            var suffix = ".app";
                            if(text.indexOf(suffix, text.length - suffix.length) !== -1) {
                                textField_cmakeFolder.text = text +fileSystemHelper.GetAdditionalCMakePath();
                            }

                            updateOutputString()
                        }
                    }
                    Connections {
                        target: rowLayout_davaFolder

                        onPathChanged: {
                            var path = rowLayout_davaFolder.path;
                            var cmakePath = fileSystemHelper.FindCMakeBin(path, davaFolderName);
                            if(cmakePath.length !== 0) {
                                rowLayout_cmakeFolder.path = cmakePath;
                            }
                        }
                    }
                }

                MutableContentItem {
                    id: mutableContent
                    onDataUpdated: {
                        updateOutputString()

                    }
                    Layout.fillWidth: true
                }
                Item {
                    id: spacer
                    height: 20
                }
                RowLayout {
                    Label {
                        id: label_customOptions
                        text: qsTr("user options")
                    }
                    TextField {
                        id: textField_customOptions
                        Layout.fillWidth: true
                        placeholderText: qsTr("your custom options")
                        onTextChanged: {
                            updateOutputString();
                        }
                    }
                }

                ColumnLayoutOutput {
                    id: columnLayoutOutput
                    Layout.fillWidth: true
                    processWrapper: processWrapper
                    buildFolder: rowLayout_buildFolder.path
                    cmakeFolder: rowLayout_cmakeFolder.path
                    property double startTime: 0
                    Component.onDestruction: { 
                        settings.outputState = columnLayoutOutput.saveState();
                    }
                    onCmakeWillBeLaunched: {
                        displayHtmlFormat = true;
                        textArea_processText.text = "";
                        addProjectToHistory();
                    }
                    onCmakeWasLaunched: {
                        //we create build folder
                        rowLayout_buildFolder.refreshPath();
                    }

                    onBuildStarted: {
                        displayHtmlFormat = false;
                        textArea_processText.text = ""
                    }
                    Connections {
                        target: processWrapper
                        onConfigureStarted: {
                            columnLayoutOutput.startTime = new Date().getTime();
                            textArea_processText.append("start time: " + Qt.formatTime(new Date(),"hh:mm:zzz"));
                        }
                        onConfigureFinished: {
                            textArea_processText.append("end configure time: " + Qt.formatTime(new Date(),"hh:mm:zzz"))
                            var diff =  new Date().getTime() - columnLayoutOutput.startTime;
                            var ms = diff % 1000;
                            var s = Math.floor((diff / 1000) % 60);
                            var m = Math.floor((diff / 1000 / 60) % 60);
                            var h = Math.floor(diff / 1000 / 60 / 60);
                            textArea_processText.append("total configure time: " + (h != 0 ? (h + ":") : "") + m + ":" + s + ":" + ms);
                        }
                    }
                }
            }
        }

        TextArea {
            id: textArea_processText
            textFormat: displayHtmlFormat ? TextEdit.RichText : TextEdit.PlainText
            readOnly: true

            Connections {
                target: processWrapper
                onProcessStateTextChanged: textArea_processText.append(displayHtmlFormat
                                                                   ? "<font color=\"DarkGreen\">" + text + "</font>"
                                                                   : "****new process state: " + text + " ****");
                onProcessErrorTextChanged: textArea_processText.append(displayHtmlFormat
                                                                   ? "<font color=\"DarkRed\">" + text + "</font>"
                                                                   : "****process error occurred!: " + text + " ****");
                onProcessStandardOutput: {
                    var maxLen = 150;
                    if(!displayHtmlFormat && text.length > maxLen) {
                        textArea_processText.append(processText(text.substring(0, maxLen) + "...\n"));
                    } else {
                        textArea_processText.append(processText(text));
                    }
                }
                onProcessStandardError: textArea_processText.append(displayHtmlFormat
                                                                    ? "<font color=\"DarkRed\">" + processText(text) + "</font>"
                                                                    : processText(text));
            }
        }
    }
}

