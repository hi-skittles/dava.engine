#pragma once

#include <QObject>
#include <QVariant>

class FileSystemHelper : public QObject
{
    Q_OBJECT

public:
    enum eErrorCode
    {
        NO_ERRORS,
        FOLDER_NAME_EMPTY,
        FOLDER_NOT_EXISTS,
        FOLDER_NOT_CONTAIN_KEY_FILE,
        CAN_NOT_REMOVE,
        CAN_NOT_CREATE_BUILD_FOLDER
    };
    explicit FileSystemHelper(QObject* parent = nullptr);
    Q_INVOKABLE QString ResolveUrl(const QString& url) const;
    static Q_INVOKABLE QString GetDir(const QString& path);
    static Q_INVOKABLE QString NormalizePath(const QString& path);
    static bool MkPath(const QString& path);
    static Q_INVOKABLE bool IsDirExists(const QString& dirPath);
    Q_INVOKABLE bool IsFileExists(const QString& filePath) const;
    Q_INVOKABLE QString FindCMakeBin(const QString& pathToDavaFramework, const QString& frameworkDirName) const;
    static eErrorCode ClearFolderIfKeyFileExists(const QString& buildFolder, const QString& keyFile);
};
