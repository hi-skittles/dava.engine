#include "filesystemhelper.h"
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>

FileSystemHelper::FileSystemHelper(QObject* parent)
    : QObject(parent)
{
}

QString FileSystemHelper::ResolveUrl(const QString& url) const
{
    QRegularExpression regExp;
#ifdef Q_OS_MAC
    regExp.setPattern("^(file:/{2})"); //on unix systems path started with '/'
#elif defined Q_OS_WIN
    regExp.setPattern("^(file:/{3})");
#endif //Q_OS_MAC Q_OS_WIN;
    QString resolvedUrl(url);
    resolvedUrl.replace(regExp, "");
    return resolvedUrl;
}

QString FileSystemHelper::GetDir(const QString& path)
{
    QFileInfo fileInfo(path);
    if (fileInfo.isDir())
    {
        return fileInfo.absoluteFilePath();
    }
    else if (fileInfo.isFile())
    {
        return fileInfo.absolutePath();
    }
    else
    {
        return QString();
    }
}

QString FileSystemHelper::NormalizePath(const QString& path)
{
    if (path.isEmpty())
    {
        return path;
    }
    QFileInfo fileInfo(path);
    if (!fileInfo.exists())
    {
        return fileInfo.absoluteFilePath();
    }

    return fileInfo.canonicalFilePath();
}

bool FileSystemHelper::MkPath(const QString& path)
{
    if (path.isEmpty())
    {
        return false;
    }
    QDir dir(path);
    return dir.mkpath(".");
}

bool FileSystemHelper::IsDirExists(const QString& dirPath)
{
    if (dirPath.isEmpty())
    {
        return false;
    }
    QDir dir(dirPath);
    return dir.exists();
}

bool FileSystemHelper::IsFileExists(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    return fileInfo.isFile() && fileInfo.exists();
}

QString FileSystemHelper::FindCMakeBin(const QString& path, const QString& frameworkDirName) const
{
    int index = path.indexOf(frameworkDirName);
    if (index == -1)
    {
        return "";
    }
    QString davaPath = path.left(index + frameworkDirName.length());
    QString cmakePath = davaPath + "/Bin" +
#ifdef Q_OS_MAC
    "/CMakeMac/CMake.app/Contents/bin/cmake";
#elif defined Q_OS_WIN
    "/CMakeWin32/bin/cmake.exe";
#endif //Q_OS_MAC Q_OS_WIN
    if (!QFile::exists(cmakePath))
    {
        return "";
    }
    return QDir::fromNativeSeparators(cmakePath);
}

FileSystemHelper::eErrorCode FileSystemHelper::ClearFolderIfKeyFileExists(const QString& folderPath, const QString& keyFile)
{
    if (folderPath.isEmpty())
    {
        return FOLDER_NAME_EMPTY;
    }
    QDir dir(folderPath);
    if (!dir.exists())
    {
        return FOLDER_NOT_EXISTS;
    }
    if (dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries).count() == 0)
    {
        return NO_ERRORS;
    }

    if (!dir.exists(keyFile))
    {
        return FOLDER_NOT_CONTAIN_KEY_FILE;
    }

    if (dir.removeRecursively())
    {
        if (dir.mkpath(folderPath))
        {
            return NO_ERRORS;
        }
        else
        {
            return CAN_NOT_CREATE_BUILD_FOLDER;
        }
    }
    return CAN_NOT_REMOVE;
}
