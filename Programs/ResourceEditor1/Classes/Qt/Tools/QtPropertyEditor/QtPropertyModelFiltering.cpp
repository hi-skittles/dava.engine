#include "QtPropertyModelFiltering.h"
#include "QtPropertyData.h"

QtPropertyModelFiltering::QtPropertyModelFiltering(QtPropertyModel* _propModel, QObject* parent /* = NULL */)
    : QSortFilterProxyModel(parent)
    , propModel(_propModel)
{
    setSourceModel(propModel);

    QObject::connect(propModel, SIGNAL(PropertyEdited(const QModelIndex&)), this, SLOT(OnPropertyEdited(const QModelIndex&)));
    QObject::connect(propModel, SIGNAL(PropertyChanged(const QModelIndex&)), this, SLOT(OnPropertyChanged(const QModelIndex&)));
    QObject::connect(propModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(OnRowsInserted(const QModelIndex&, int, int)));
    QObject::connect(propModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), this, SLOT(OnRowsRemoved(const QModelIndex&, int, int)));
}

bool QtPropertyModelFiltering::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (NULL != propModel)
    {
        // check self accept
        if (selfAcceptRow(sourceRow, sourceParent))
        {
            return true;
        }

        //accept if any of the parents is accepted
        QModelIndex parent = sourceParent;
        while (parent.isValid())
        {
            if (selfAcceptRow(parent.row(), parent.parent()))
            {
                return true;
            }

            parent = parent.parent();
        }

        // accept if any child is accepted
        if (childrenAcceptRow(sourceRow, sourceParent))
        {
            return true;
        }
    }

    return false;
}

bool QtPropertyModelFiltering::selfAcceptRow(int sourceRow, const QModelIndex& sourceParent) const
{
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

bool QtPropertyModelFiltering::childrenAcceptRow(int sourceRow, const QModelIndex& sourceParent) const
{
    bool ret = false;

    QModelIndex index = propModel->index(sourceRow, 0, sourceParent);
    if (propModel->rowCount(index) > 0)
    {
        for (int i = 0; i < propModel->rowCount(index); i++)
        {
            if (selfAcceptRow(i, index) || childrenAcceptRow(i, index))
            {
                ret = true;
                break;
            }
        }
    }

    return ret;
}

void QtPropertyModelFiltering::OnPropertyEdited(const QModelIndex& index)
{
    emit PropertyEdited(mapFromSource(index));
}

void QtPropertyModelFiltering::OnPropertyChanged(const QModelIndex& index)
{
    emit PropertyChanged(mapFromSource(index));
}

void QtPropertyModelFiltering::OnRowsInserted(const QModelIndex&, int, int)
{
    invalidate();
}

void QtPropertyModelFiltering::OnRowsRemoved(const QModelIndex&, int, int)
{
    invalidate();
}
