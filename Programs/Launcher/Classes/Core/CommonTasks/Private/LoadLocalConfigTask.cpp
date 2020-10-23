#include "Core/CommonTasks/LoadLocalConfigTask.h"
#include "Core/ApplicationContext.h"
#include "Core/ConfigHolder.h"

#include "Gui/MainWindow.h"

#include "Data/ConfigParser.h"
#include <QFile>

LoadLocalConfigTask::LoadLocalConfigTask(ApplicationContext* appContext, ConfigHolder* configHolder_, const QString& localConfigPath_)
    : RunTask(appContext)
    , localConfigPath(localConfigPath_)
    , configHolder(configHolder_)
{
}

QString LoadLocalConfigTask::GetDescription() const
{
    return QObject::tr("Loading local config");
}

void LoadLocalConfigTask::Run()
{
    QFile configFile(localConfigPath);
    if (!configFile.exists())
    {
        return;
    }
    configHolder->localConfig.Clear();
    if (configFile.open(QFile::ReadOnly))
    {
        QByteArray data = configFile.readAll();
        configFile.close();
        if (data.isEmpty() == false)
        {
            configHolder->localConfig.ParseJSON(data, this);
            configHolder->localConfig.UpdateApplicationsNames();

            //kostil
            Branch* branch = configHolder->localConfig.GetBranch("ST");
            if (branch != nullptr)
            {
                branch->id = "Stable";
            }
            branch = configHolder->localConfig.GetBranch("BLITZTOSTABLE");
            if (branch != nullptr)
            {
                branch->id = "Blitz To Stable";
            }
        }
    }
    else
    {
        SetError(QObject::tr("Can not load local config: file %1 is inaccessible").arg(localConfigPath));
    }
}
