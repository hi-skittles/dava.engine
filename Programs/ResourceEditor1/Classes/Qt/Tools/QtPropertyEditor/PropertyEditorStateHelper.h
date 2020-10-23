#ifndef __PROPERTY_EDITOR_STATE_HELPER__H__
#define __PROPERTY_EDITOR_STATE_HELPER__H__

#include "Main/QTreeViewStateHelper.h"
#include "Tools/QtPropertyEditor/QtPropertyModel.h"
#include "Tools/QtPropertyEditor/QtPropertyData.h"

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

#endif /* defined(__PROPERTY_EDITOR_STATE_HELPER__H__) */
