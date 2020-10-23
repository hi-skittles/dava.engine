import QtQuick 2.6
import QtQuick.Controls 1.5
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3
import Cpp.Utils 1.0

RowLayout {
    id: rowLayout
    property alias labelText: label.text
    property alias dialogTitle: fileDialog.title
    property bool selectFolders: false;
    property alias inputComponent: loader.sourceComponent
    property string path: loader.item.text
    property alias item: loader.item
    Binding { target: rowLayout; property: "path"; value: loader.item.text}
    Binding { target: loader.item; property: "text"; value: path }
    onPathChanged: loader.item.text = path
    Layout.fillWidth: true
    property bool pathIsValid: selectFolders
                               ? fileSystemHelper.IsDirExists(loader.item.text)
                               : fileSystemHelper.IsFileExists(loader.item.text)

    //refresh exists icon when path is the same
    function refreshPath()
    {
        var txt = loader.item.text;
        loader.item.text = "";
        loader.item.text = txt;
    }

    Label {
        id: label
    }

    Loader {
        id: loader
        Layout.fillWidth: true
    }

    FileSystemHelper {
        id: fileSystemHelper
    }

    FileDialog {
        id: fileDialog
        selectFolder: selectFolders
        onAccepted: {
            var url = fileDialog.fileUrls[0].toString()
            url = fileSystemHelper.ResolveUrl(url);
            loader.item.text = url;
            refreshPath();
        }
    }

    Button {
        id: button
        iconSource: "qrc:///Icons/openfolder.png"
        onClicked: {
            var folder = loader.item.text;
            if(folder.length !== 0) {
                if(folder[0] === "/") {
                    folder = folder.substring(1);
                }
                fileDialog.folder = "file:///" + fileSystemHelper.GetDir(folder);
            }

            fileDialog.open();
        }
    }

    Image {
        id: image
        width: height
        height: parent.height
        source: "qrc:///Icons/" + (pathIsValid ? "ok" : "error") + ".png"
    }
}
