#include "Core/CommonTasks/RunApplicationTask.h"
#include "Core/ApplicationContext.h"
#include "Core/ConfigHolder.h"

#include "Data/ConfigParser.h"

#include "Utils/Utils.h"

#include <QtHelpers/ProcessHelper.h>

#include <QObject>
#include <QFile>

namespace RunApplicationTaskDetails
{
#ifdef Q_OS_WIN
void FixLocalAppPath_kostil(QString& runPath)
{
    if (QFile::exists(runPath) == false)
    {
        //Standalone ResourceEditor kostil
        //BA-manager not support different executable paths
        QString newString = "Programs/ResourceEditor/ResourceEditor.exe";
        QString oldString = "Tools/ResourceEditor/ResourceEditor.exe";
        if (runPath.endsWith(newString))
        {
            runPath.replace(newString, oldString);
        }
    }
}
#endif //Q_OS_WIN
}

RunApplicationTask::RunApplicationTask(ApplicationContext* appContext, ConfigHolder* configHolder_, const QString& branch_, const QString& app_, const QString& version_)
    : RunTask(appContext)
    , branch(branch_)
    , app(app_)
    , version(version_)
    , configHolder(configHolder_)
{
}

QString RunApplicationTask::GetDescription() const
{
    return QObject::tr("Launching application %1").arg(app);
}

void RunApplicationTask::Run()
{
    AppVersion* localVersion = configHolder->localConfig.GetAppVersion(branch, app, version);
    if (localVersion == nullptr)
    {
        SetError(QObject::tr("Version %1 of application %2 is not installed").arg(version).arg(app));
        return;
    }
    QString runPath = LauncherUtils::GetApplicationDirectory(configHolder, appContext, branch, app, localVersion->isToolSet);
    if (QFile::exists(runPath) == false)
    {
        SetError(QObject::tr("Application %1 in branch %2 not exists!").arg(app).arg(branch));
        return;
    }
    QString localAppPath = LauncherUtils::GetLocalAppPath(localVersion, app);
    runPath += localAppPath;
#ifdef Q_OS_WIN
    RunApplicationTaskDetails::FixLocalAppPath_kostil(runPath);
#endif //Q_OS_WIN
    if (!QFile::exists(runPath))
    {
        SetError(QObject::tr("Application not found: %1").arg(runPath));
        return;
    }

    QString appName = runPath.right(runPath.size() - runPath.lastIndexOf("/") - 1);

    ProcessHelper::RunProcess(runPath);
}
