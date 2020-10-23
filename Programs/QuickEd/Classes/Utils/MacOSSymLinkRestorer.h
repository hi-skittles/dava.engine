#pragma once

#include <TArc/Qt/QtString.h>

#include <QVector>

class MacOSSymLinkRestorer
{
public:
    MacOSSymLinkRestorer(const QString& directory);

    QString RestoreSymLinkInFilePath(const QString& origFilePath) const;

private:
    QVector<QPair<QString, QString>> symlinks; // first: real path, second: symbolic link
};