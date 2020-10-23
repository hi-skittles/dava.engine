#include "Utils/Utils.h"
#include "Utils/FileManager.h"

#include "Data/ConfigParser.h"

#include "Core/ApplicationContext.h"
#include "Core/ConfigHolder.h"

#include <QDebug>
#include <QRegularExpression>
#include <QFile>

#include <assert.h>

namespace LauncherUtilsDetails
{
template <typename T>
T* FindItemWithName(QList<T>& cont, const QString& name)
{
    QList<T*> items;
    for (T& item : cont)
    {
        if (item.id.contains(name, Qt::CaseInsensitive))
        {
            items.push_back(&item);
        }
    }
    if (items.isEmpty())
    {
        return nullptr;
    }
    else if (items.size() == 1)
    {
        return items.front();
    }
    else
    {
        QList<T*> foundItems;
        for (T* item : items)
        {
            if (item->id.compare(name, Qt::CaseInsensitive) == 0)
            {
                foundItems.push_back(item);
            }
        }
        if (foundItems.isEmpty())
        {
            return nullptr;
        }
        else if (foundItems.size() == 1)
        {
            return foundItems.front();
        }
        else
        {
            foundItems.clear();
            for (T* item : items)
            {
                if (item->id.compare(name, Qt::CaseSensitive) == 0)
                {
                    foundItems.push_back(item);
                }
            }

            if (foundItems.size() == 1)
            {
                return foundItems.front();
            }
            else
            {
                return nullptr;
            }
        }
    }
}

QString RemoveWhitespace(const QString& str)
{
    QString replacedStr = str;
    QRegularExpression spaceRegex("\\s+");
    replacedStr.replace(spaceRegex, "");
    return replacedStr;
}

QString GetApplicationDirectory_kostil(const ApplicationContext* context, const QString& branchName, const QString& appName)
{
    QString path = context->fileManager.GetBaseAppsDirectory() + branchName + "/" + appName + "/";
    return path;
}
}

QString LauncherUtils::GetAppName(const QString& appName, bool isToolSet)
{
    return isToolSet ? "Toolset" : appName;
}

QString LauncherUtils::GetLocalAppPath(const AppVersion* version, const QString& appName)
{
    Q_ASSERT(!appName.isEmpty());
    if (version == nullptr)
    {
        return QString();
    }
    QString runPath = version->runPath;
    if (runPath.isEmpty())
    {
        QString correctID = LauncherUtilsDetails::RemoveWhitespace(appName);
#ifdef Q_OS_WIN
        runPath = correctID + ".exe";
#elif defined(Q_OS_MAC)
        runPath = correctID + ".app";
#else
#error "unsupported platform"
#endif //platform
    }
    return runPath;
}

QString LauncherUtils::GetApplicationDirectory(const ConfigHolder* configHolder, const ApplicationContext* context, QString branchName, QString appName, bool isToolSet)
{
    appName = LauncherUtils::GetAppName(appName, isToolSet);

    branchName = LauncherUtilsDetails::RemoveWhitespace(branchName);
    appName = LauncherUtilsDetails::RemoveWhitespace(appName);

    //try to get right path
    QString runPath = context->fileManager.GetApplicationDirectory(branchName, appName);
    if (QFile::exists(runPath))
    {
        return runPath;
    }

    //try to get old ugly path with a bug on "/" symbol
    QString tmpRunPath = LauncherUtilsDetails::GetApplicationDirectory_kostil(context, branchName, appName);
    if (QFile::exists(tmpRunPath))
    {
        return tmpRunPath;
    }
    //we can have old branch name or old app name
    QList<QString> branchKeys = configHolder->localConfig.GetStrings().keys(branchName);
    //it can be combination of old and new names
    branchKeys.append(branchName);
    for (const QString& branchKey : branchKeys)
    {
        QList<QString> appKeys = configHolder->localConfig.GetStrings().keys(appName);
        appKeys.append(appName);
        for (const QString& appKey : appKeys)
        {
            QString newRunPath = context->fileManager.GetApplicationDirectory(branchKey, appKey);
            if (QFile::exists(newRunPath))
            {
                return newRunPath;
            }
            newRunPath = LauncherUtilsDetails::GetApplicationDirectory_kostil(context, branchKey, appKey);
            if (QFile::exists(newRunPath))
            {
                return newRunPath;
            }
        }
    }
    return runPath;
}

bool LauncherUtils::CanTryStopApplication(const QString& applicationName)
{
    return applicationName.contains("assetcacheserver", Qt::CaseInsensitive);
}

Branch* LauncherUtils::FindBranch(ConfigParser* config, const QString& branchName)
{
    assert(config != nullptr);
    return LauncherUtilsDetails::FindItemWithName(config->GetBranches(), branchName);
}

Application* LauncherUtils::FindApplication(Branch* branch, QString appName)
{
    assert(branch != nullptr);
    return LauncherUtilsDetails::FindItemWithName(branch->applications, appName);
}

AppVersion* LauncherUtils::FindVersion(Application* app, QString versionName)
{
    assert(app != nullptr);
    if (versionName.isEmpty() || versionName.compare("recent", Qt::CaseInsensitive) == 0)
    {
        if (app->versions.isEmpty())
        {
            return nullptr;
        }
        else
        {
            return &app->versions.back();
        }
    }

    return LauncherUtilsDetails::FindItemWithName(app->versions, versionName);
}

Application* LauncherUtils::FindApplication(ConfigParser* config, const QString& branchName, const QString& appName)
{
    Branch* branch = FindBranch(config, branchName);
    if (branch != nullptr)
    {
        return FindApplication(branch, appName);
    }
    return nullptr;
}

AppVersion* LauncherUtils::FindVersion(Branch* branch, const QString& appName, const QString& versionName)
{
    Application* app = FindApplication(branch, appName);
    if (app != nullptr)
    {
        return FindVersion(app, versionName);
    }

    return nullptr;
}

AppVersion* LauncherUtils::FindVersion(ConfigParser* config, const QString& branchName, const QString& appName, const QString& versionName)
{
    Branch* branch = FindBranch(config, branchName);
    if (branch != nullptr)
    {
        return FindVersion(branch, appName, versionName);
    }
    return nullptr;
}
