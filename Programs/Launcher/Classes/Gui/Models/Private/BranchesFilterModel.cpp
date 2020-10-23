#include "Gui/Models/BranchesFilterModel.h"
#include "Gui/Models/BranchesListModel.h"

BranchesFilterModel::BranchesFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

bool BranchesFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    BranchesListModel* model = dynamic_cast<BranchesListModel*>(sourceModel());
    if (model != nullptr)
    {
        BranchesListModel::ListItemType type = model->GetType(source_row);
        if (type == BranchesListModel::LIST_ITEM_FAVORITES || type == BranchesListModel::LIST_ITEM_SEPARATOR)
        {
            return true;
        }
    }
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}
