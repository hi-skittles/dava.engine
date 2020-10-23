#include "LibraryFileSystemModel.h"

#include "Logger/Logger.h"

#include <QDir>
#include <QFileInfoList>
#include <QString>

LibraryFileSystemModel::LibraryFileSystemModel(QObject* parent /* = NULL */)
    : QFileSystemModel(parent)
    , loadingCounter(0)
{
    SetExtensionFilter(QStringList());

    connect(this, SIGNAL(directoryLoaded(const QString&)), this, SLOT(DirectoryLoaded(const QString&)));
}

void LibraryFileSystemModel::SetExtensionFilter(const QStringList& extensionFilter)
{
    setNameFilters(extensionFilter);
    setNameFilterDisables(false);

    //magic code for MacOS
    setFilter(QDir::Files);
    setFilter(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::AllDirs);
}

void LibraryFileSystemModel::DirectoryLoaded(const QString& path)
{
    //disabled for future
    //     QDir dir(path);
    //     QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    //
    //     auto endIt = entries.end();
    //     for (auto it = entries.begin(); it != endIt; ++it)
    //     {
    //         QModelIndex ind = index(it->absoluteFilePath());
    //         if(ind.isValid())
    //         {
    //             if(canFetchMore(ind))
    //             {
    //                 ++loadingCounter;
    //                 fetchMore(ind);
    //             }
    //         }
    //     }
    //
    //     --loadingCounter;
    //     if(loadingCounter == 0)
    //     {
    //         emit ModelLoaded();
    //     }
}

void LibraryFileSystemModel::Load(const QString& pathname)
{
    acceptionMap.clear();
    //reset();

    loadingCounter = 1;

    setRootPath(pathname);
}

void LibraryFileSystemModel::SetAccepted(const QModelIndex& index, bool accepted)
{
    if (index.isValid())
    {
        acceptionMap[fileInfo(index).absoluteFilePath()] = accepted;
    }
}

bool LibraryFileSystemModel::IsAccepted(const QModelIndex& index) const
{
    if (index.isValid())
    {
        return acceptionMap[fileInfo(index).absoluteFilePath()].toBool();
    }

    return false;
}

QVariant LibraryFileSystemModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::BackgroundColorRole && IsAccepted(index))
    {
        return QColor(0, 255, 0, 20);
    }

    return QFileSystemModel::data(index, role);
}

bool LibraryFileSystemModel::HasFilteredChildren(const QModelIndex& index)
{
    if (!index.isValid())
        return false;

    QFileInfo info = fileInfo(index);
    if (info.isDir())
    {
        QDir dir = info.filePath();

        dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
        dir.setNameFilters(nameFilters());

        int filesCount = dir.count();
        if (filesCount > 0)
            return true;

        for (int i = 0; i < rowCount(index); ++i)
        {
            bool hasChildren = HasFilteredChildren(this->index(i, 0, index));
            if (hasChildren)
                return true;
        }
    }

    return false;
}
