#pragma once

#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyModel.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData.h"
#include "Classes/Qt/Main/QTreeViewStateHelper.h"

class PropertyEditorStateHelper : public DAVA::QTreeViewStateHelper<QString>
{
public:
    virtual void SaveTreeViewState(bool needCleanupStorage);
    PropertyEditorStateHelper(QTreeView* treeView, QtPropertyModel* model);

protected:
    virtual QString GetPersistentDataForModelIndex(const QModelIndex& modelIndex);

private:
    QtPropertyModel* model;
};
