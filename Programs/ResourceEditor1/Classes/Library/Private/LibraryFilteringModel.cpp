#include "LibraryFilteringModel.h"
#include "LibraryFileSystemModel.h"

#include "Logger/Logger.h"

LibraryFilteringModel::LibraryFilteringModel(QObject* parent /* = NULL */)
    : QSortFilterProxyModel(parent)
    , model(NULL)
{
}

void LibraryFilteringModel::SetModel(LibraryFileSystemModel* newModel)
{
    model = newModel;
    setSourceModel(model);
}

bool LibraryFilteringModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (NULL == model)
        return false;

    if (filterRegExp().isEmpty())
    {
        return EmptyFilterAccept(sourceRow, sourceParent);
    }
    else
    {
        return FilterAccept(sourceRow, sourceParent);
    }
}

bool LibraryFilteringModel::EmptyFilterAccept(int sourceRow, const QModelIndex& sourceParent) const
{
    //reset acception to avoid coloring background
    QModelIndex index = model->index(sourceRow, 0, sourceParent);
    model->SetAccepted(index, false);

    bool accepted = selfAcceptRow(sourceRow, sourceParent);
    if (accepted)
    {
        if (HideEmptyFolder(index))
            return false;
    }

    return accepted;
}

bool LibraryFilteringModel::FilterAccept(int sourceRow, const QModelIndex& sourceParent) const
{
    bool accepted = selfAcceptRow(sourceRow, sourceParent);

    QModelIndex index = model->index(sourceRow, 0, sourceParent);
    model->SetAccepted(index, accepted);

    if (!accepted)
    {
        accepted = childrenAcceptRow(sourceRow, sourceParent);
        if (!accepted)
        {
            if (!HideEmptyFolder(index))
            {
                accepted = parentAcceptRow(sourceParent);
            }
        }
    }

    return accepted;
}

bool LibraryFilteringModel::HideEmptyFolder(const QModelIndex& index) const
{
    bool isDir = model->isDir(index);
    return (isDir && !model->HasFilteredChildren(index));
}

bool LibraryFilteringModel::selfAcceptRow(int sourceRow, const QModelIndex& sourceParent) const
{
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    ;
}

bool LibraryFilteringModel::childrenAcceptRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index = model->index(sourceRow, 0, sourceParent);

    for (int i = 0; i < model->rowCount(index); ++i)
    {
        if (selfAcceptRow(i, index) || childrenAcceptRow(i, index))
        {
            return true;
        }
    }

    return false;
}

bool LibraryFilteringModel::parentAcceptRow(const QModelIndex& sourceParent) const
{
    QModelIndex parent = sourceParent;
    while (parent.isValid())
    {
        if (selfAcceptRow(parent.row(), parent.parent()))
        {
            return true;
        }

        parent = parent.parent();
    }

    return false;
}
