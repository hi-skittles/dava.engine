#include "Core/CommonTasks/InstallApplicationTask.h"
#include "Core/ApplicationContext.h"
#include "Core/CommonTasks/DownloadTask.h"
#include "Core/CommonTasks/RemoveApplicationTask.h"
#include "Core/CommonTasks/UnzipTask.h"
#include "Core/CommonTasks/RunApplicationTask.h"
#include "Core/ConfigHolder.h"

#include "Utils/AppsCommandsSender.h"
#include "Utils/FileManager.h"
#include "Utils/Utils.h"

#include <QNetworkReply>
#include <QEventLoop>

InstallApplicationTask::InstallApplicationTask(ApplicationContext* appContext, ConfigHolder* configHolder_, const InstallApplicationParams& params_)
    : AsyncChainTask(appContext)
    , params(params_)
    , configHolder(configHolder_)
{
}

QString InstallApplicationTask::GetDescription() const
{
    return QObject::tr("Installing %1 from branch %2")
    .arg(LauncherUtils::GetAppName(params.app, params.newVersion.isToolSet))
    .arg(params.branch);
}

void InstallApplicationTask::Run()
{
    //replace this code later with "Create file while loading"
    QString filePath = appContext->fileManager.GetTempDownloadFilePath(params.newVersion.url);
    fileToWrite.setFileName(filePath);
    if (fileToWrite.open(QIODevice::WriteOnly | QIODevice::Truncate) == false)
    {
        SetError(QObject::tr("Can not create file %1!").arg(filePath));
        emit Finished();
        return;
    }

    QString description = QObject::tr("Downloading %1 from branch %2")
                          .arg(LauncherUtils::GetAppName(params.app, params.newVersion.isToolSet))
                          .arg(params.branch);
    std::unique_ptr<BaseTask> task = appContext->CreateTask<DownloadTask>(description, params.newVersion.url, &fileToWrite);

    Receiver receiver;
    receiver.onFinished = [this](const BaseTask* task) {
        if (task->HasError())
        {
            emit Finished();
            return;
        }
        else
        {
            OnLoaded();
        }
    };

    appContext->taskManager.AddTask(std::move(task), Notifier({ transparentReceiver, receiver }));
}

int InstallApplicationTask::GetSubtasksCount() const
{
    return NeedUnpack() ? 2 : 1;
}

void InstallApplicationTask::OnLoaded()
{
    QString filePath = appContext->fileManager.GetTempDownloadFilePath(params.newVersion.url);

    fileToWrite.close();
    if (fileToWrite.size() <= 0)
    {
        SetError(QObject::tr("Failed to write file %1!").arg(filePath));
        emit Finished();
        return;
    }

    applicationsToRestart = GetApplicationsToRestart(params.branch, params.app);

    //remove application if was installed
    Application* localApp = configHolder->localConfig.GetApplication(params.branch, params.app);
    if (localApp != nullptr)
    {
        AppVersion* localVersion = localApp->GetVersion(0);
        if (localVersion != nullptr)
        {
            std::unique_ptr<BaseTask> task = appContext->CreateTask<RemoveApplicationTask>(configHolder, params.branch, params.app);
            appContext->taskManager.AddTask(std::move(task), transparentReceiver);
        }
    }

    Install();
}

void InstallApplicationTask::Install()
{
    QString filePath = appContext->fileManager.GetTempDownloadFilePath(params.newVersion.url);

    QString appDirPath = LauncherUtils::GetApplicationDirectory(configHolder, appContext, params.branch, params.app, params.newVersion.isToolSet);
    if (QFile::exists(appDirPath) == false)
    {
        FileManager::MakeDirectory(appDirPath);
    }

    if (NeedUnpack())
    {
        Receiver receiver;
        receiver.onFinished = [this](const BaseTask* task) {
            if (task->HasError())
            {
                emit Finished();
            }
            else
            {
                OnInstalled();
            }
        };

        std::unique_ptr<BaseTask> task = appContext->CreateTask<UnzipTask>(filePath, appDirPath);
        appContext->taskManager.AddTask(std::move(task), Notifier({ transparentReceiver, receiver }));
        return;
    }
    else
    {
        QString newFilePath = appDirPath + appContext->fileManager.GetFileNameFromURL(params.newVersion.url);
        if (appContext->fileManager.MoveFileWithMakePath(filePath, newFilePath))
        {
            OnInstalled();
        }
        else
        {
            SetError(QObject::tr("Moving downloaded application failed"));
            emit Finished();
        }
    }
}

void InstallApplicationTask::OnInstalled()
{
    configHolder->localConfig.InsertApplication(params.branch, params.app, params.newVersion);
    configHolder->localConfig.UpdateApplicationsNames();
    configHolder->localConfig.SaveToFile(FileManager::GetLocalConfigFilePath());

    FileManager::DeleteDirectory(appContext->fileManager.GetTempDirectory());
    for (const QString& appToRestart : applicationsToRestart)
    {
        AppVersion* installedAppVersion = configHolder->localConfig.GetAppVersion(params.branch, appToRestart);
        appContext->taskManager.AddTask(appContext->CreateTask<RunApplicationTask>(configHolder, params.branch, appToRestart, installedAppVersion->id), transparentReceiver);
    }

    if (params.appToStart.isEmpty() == false)
    {
        Application* app = LauncherUtils::FindApplication(&configHolder->localConfig, params.branch, params.appToStart);
        if (app != nullptr)
        {
            AppVersion* version = LauncherUtils::FindVersion(app, QString());
            if (version != nullptr)
            {
                appContext->taskManager.AddTask(appContext->CreateTask<RunApplicationTask>(configHolder, params.branch, app->id, version->id), transparentReceiver);
            }
        }
    }
    emit Finished();
}

QStringList InstallApplicationTask::GetApplicationsToRestart(const QString& branchID, const QString& appID)
{
    QStringList appList;
    const AppVersion* installedVersion = configHolder->localConfig.GetAppVersion(branchID, appID);

    if (installedVersion == nullptr)
    {
        return appList;
    }

    QStringList appNames;
    if (installedVersion->isToolSet)
    {
        QStringList toolsetApps = configHolder->localConfig.GetTranslatedToolsetApplications();
        for (auto iter = toolsetApps.begin(); iter != toolsetApps.end(); ++iter)
        {
            appNames << *iter;
        }
    }
    else
    {
        appNames << appID;
    }

    for (const QString& appName : appNames)
    {
        if (LauncherUtils::CanTryStopApplication(appName))
        {
            QString dirPath = LauncherUtils::GetApplicationDirectory(configHolder, appContext, branchID, appID, installedVersion->isToolSet);
            QString localPath = LauncherUtils::GetLocalAppPath(installedVersion, appID);
            if (appContext->appsCommandsSender.HostIsAvailable(dirPath + localPath))
            {
                appList << appName;
            }
        }
    }
    return appList;
}

bool InstallApplicationTask::NeedUnpack() const
{
    return params.newVersion.url.endsWith("zip");
}
