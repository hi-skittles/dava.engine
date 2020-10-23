import QtQuick 2.6
import QtQuick.Controls 1.5

ComboBox {
    id: comboBox_buildFolder
    editable: true

    property alias text: comboBox_buildFolder.editText
    model: ListModel {
        id: listModel
    }
    function addString(text) {
        var obj = {"text" : text};
        listModel.append({"text" : text});
        currentIndex = listModel.count - 1
    }
    onTextChanged: {
        for(var i = 0, length = listModel.count; i < length; ++i) {
            var obj = listModel.get(i);
            if(obj && obj.text && obj.text === text) {
                currentIndex = i;
                break;
            }
        }
    }
}
