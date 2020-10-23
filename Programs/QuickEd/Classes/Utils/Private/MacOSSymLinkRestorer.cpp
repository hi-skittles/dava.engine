#include "Utils/MacOSSymLinkRestorer.h"

#include <QDirIterator>

namespace MacOSSymLinkRestorerDetails
{
QVector<QPair<QString, QString>> FindSymLinks(const QString& absDirPath)
{
    QVector<QPair<QString, QString>> symlinks;
    QDirIterator dirIterator(absDirPath, QDir::NoDotAndDotDot | QDir::Dirs | QDir::Hidden, QDirIterator::Subdirectories);
    while (dirIterator.hasNext())
    {
        dirIterator.next();
        QFileInfo fileInfo(dirIterator.fileInfo());

        if (fileInfo.isSymLink())
        {
            symlinks.push_back(qMakePair(fileInfo.absoluteFilePath(), fileInfo.symLinkTarget()));
            symlinks += FindSymLinks(fileInfo.symLinkTarget());
        }
    }

    return symlinks;
}
}

MacOSSymLinkRestorer::MacOSSymLinkRestorer(const QString& directory)
{
    symlinks = MacOSSymLinkRestorerDetails::FindSymLinks(directory);
}

QString MacOSSymLinkRestorer::RestoreSymLinkInFilePath(const QString& origFilePath) const
{
    QString filePath = origFilePath;
    for (const auto& item : MacOSSymLinkRestorer::symlinks.toStdVector())
    {
        if (filePath.startsWith(item.second))
        {
            filePath.replace(item.second, item.first);
            return RestoreSymLinkInFilePath(filePath);
        }
    }
    return filePath;
}
