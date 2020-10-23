#ifndef __LIBRARY_FILESYSTEM_FILTERING_MODEL_H__
#define __LIBRARY_FILESYSTEM_FILTERING_MODEL_H__

#include <QSortFilterProxyModel>
class LibraryFileSystemModel;
class LibraryFilteringModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    LibraryFilteringModel(QObject* parent = NULL);

    void SetModel(LibraryFileSystemModel* newModel);

protected:
    bool EmptyFilterAccept(int sourceRow, const QModelIndex& sourceParent) const;
    bool FilterAccept(int sourceRow, const QModelIndex& sourceParent) const;

    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;

    bool selfAcceptRow(int sourceRow, const QModelIndex& sourceParent) const;
    bool childrenAcceptRow(int sourceRow, const QModelIndex& sourceParent) const;
    bool parentAcceptRow(const QModelIndex& sourceParent) const;

    bool HideEmptyFolder(const QModelIndex& index) const;

protected:
    LibraryFileSystemModel* model;
};

#endif // __LIBRARY_FILESYSTEM_FILTERING_MODEL_H__
