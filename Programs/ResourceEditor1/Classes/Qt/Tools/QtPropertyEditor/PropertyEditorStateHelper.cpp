#include "PropertyEditorStateHelper.h"

PropertyEditorStateHelper::PropertyEditorStateHelper(QTreeView* treeView, QtPropertyModel* model)
    : QTreeViewStateHelper(treeView)
{
    this->model = model;
}

void PropertyEditorStateHelper::SaveTreeViewState(bool needCleanupStorage)
{
    // Need to cleanup the full paths cache before the save.
    DAVA::QTreeViewStateHelper<QString>::SaveTreeViewState(needCleanupStorage);
}

QString PropertyEditorStateHelper::GetPersistentDataForModelIndex(const QModelIndex& modelIndex)
{
    QString ret;

    QtPropertyData* data = model->itemFromIndex(modelIndex);
    if (NULL != data)
    {
        ret = data->GetPath();
    }

    return ret;
}
