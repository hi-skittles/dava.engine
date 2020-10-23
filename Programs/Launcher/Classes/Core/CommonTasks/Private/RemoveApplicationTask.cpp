#include "Core/CommonTasks/RemoveApplicationTask.h"
#include "Core/ApplicationContext.h"
#include "Core/ConfigHolder.h"

#include "Utils/AppsCommandsSender.h"
#include "Utils/FileManager.h"
#include "Utils/Utils.h"

#include <QtHelpers/ProcessHelper.h>
#include <QtHelpers/HelperFunctions.h>

#include <QEventLoop>
#include <QElapsedTimer>

#include <atomic>
#include <thread>

RemoveApplicationTask::RemoveApplicationTask(ApplicationContext* appContext, ConfigHolder* configHolder_, const QString& branch_, const QString& app_)
    : RunTask(appContext)
    , branch(branch_)
    , app(app_)
    , configHolder(configHolder_)
{
}

QString RemoveApplicationTask::GetDescription() const
{
    Application* localApp = configHolder->localConfig.GetApplication(branch, app);
    if (localApp != nullptr)
    {
        AppVersion* localVersion = localApp->GetVersion(0);
        if (localVersion != nullptr)
        {
            return QObject::tr("Removing application %1").arg(LauncherUtils::GetAppName(app, localVersion->isToolSet));
        }
    }
    return QObject::tr("Removing application %1").arg(app);
}

void RemoveApplicationTask::Run()
{
    Application* localApp = configHolder->localConfig.GetApplication(branch, app);
    if (localApp == nullptr)
    {
        return;
    }
    AppVersion* localVersion = localApp->GetVersion(0);
    if (localVersion == nullptr)
    {
        return;
    }
    QStringList appNames;
    bool isToolSet = localVersion->isToolSet;
    if (isToolSet)
    {
        appNames << configHolder->localConfig.GetTranslatedToolsetApplications();
    }
    else
    {
        appNames << app;
    }

    using appWorkerFn = std::function<bool(const QString&, const QString&, AppVersion*)>;
    auto forEachApp = [&appNames, this](appWorkerFn fn) -> bool {
        for (const QString& fakeName : appNames)
        {
            Application* fakeApp = configHolder->localConfig.GetApplication(branch, fakeName);
            if (fakeApp != nullptr)
            {
                AppVersion* fakeVersion = fakeApp->GetVersion(0);
                if (fakeVersion != nullptr)
                {
                    if (fn(branch, fakeName, fakeVersion) == false)
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    };

    using namespace std::placeholders;
    if (forEachApp(std::bind(&RemoveApplicationTask::CanRemoveApp, this, _1, _2, _3)))
    {
        forEachApp(std::bind(&RemoveApplicationTask::RemoveApplicationImpl, this, _1, _2, _3));
    }
}

bool RemoveApplicationTask::CanRemoveApp(const QString& branchID, const QString& appID, AppVersion* localVersion) const
{
    QString appDirPath = LauncherUtils::GetApplicationDirectory(configHolder, appContext, branchID, appID, localVersion->isToolSet);
    if (QFile::exists(appDirPath) == false)
    {
        SetError(QObject::tr("Can not find application %1 in branch %2").arg(appID).arg(branchID));
        return true;
    }

    QString runPath = appDirPath + LauncherUtils::GetLocalAppPath(localVersion, appID);
    if (appContext->appsCommandsSender.HostIsAvailable(runPath))
    {
        if (LauncherUtils::CanTryStopApplication(appID))
        {
            if (TryStopApp(runPath))
            {
                return true;
            }
        }
    }

    if (ProcessHelper::IsProcessRuning(runPath))
    {
        SetError(QObject::tr("Can not remove application %1 in branch %2 because it still running").arg(appID, branchID));
        return false;
    }
    return true;
}

bool RemoveApplicationTask::TryStopApp(const QString& runPath) const
{
    using namespace std;
    if (appContext->appsCommandsSender.RequestQuit(runPath))
    {
        bool isStillRunning = true;
        QElapsedTimer timer;
        timer.start();
        const int maxWaitTimeMs = 10 * 1000;
        while (timer.elapsed() < maxWaitTimeMs && isStillRunning)
        {
            const int waitTimeMs = 100;
            this_thread::sleep_for(std::chrono::milliseconds(waitTimeMs));
            isStillRunning = ProcessHelper::IsProcessRuning(runPath);
        }
        return isStillRunning == false;
    }
    return false;
}

bool RemoveApplicationTask::RemoveApplicationImpl(const QString& branchID, const QString& appID, AppVersion* localVersion)
{
    QString appDirPath = LauncherUtils::GetApplicationDirectory(configHolder, appContext, branchID, appID, localVersion->isToolSet);
    if (QFile::exists(appDirPath) && FileManager::DeleteDirectory(appDirPath) == false)
    {
        SetError(QObject::tr("Can not remove directory %1\nApplication need to be reinstalled!\n"
                             "If this error occurs again - remove directory by yourself, please")
                 .arg(appDirPath));
        return false;
    }
    configHolder->localConfig.RemoveApplication(branchID, appID, localVersion->id);
    configHolder->localConfig.SaveToFile(FileManager::GetLocalConfigFilePath());

    return true;
}
