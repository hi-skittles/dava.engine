#pragma once

#include "Core/CommonTasks/AsyncChainTask.h"
#include "Data/ConfigParser.h"

#include <QFile>

struct ConfigHolder;

struct InstallApplicationParams
{
    QString branch;
    QString app;
    AppVersion newVersion;
    QString appToStart;
};

class InstallApplicationTask final : public AsyncChainTask
{
public:
    InstallApplicationTask(ApplicationContext* appContext, ConfigHolder* configHolder, const InstallApplicationParams& params);

private:
    QString GetDescription() const override;
    void Run() override;

    int GetSubtasksCount() const override;

    void OnLoaded();
    void Install();
    void OnInstalled();

    QStringList GetApplicationsToRestart(const QString& branchID, const QString& appID);
    bool CanTryStopApplication(const QString& applicationName) const;

    bool NeedUnpack() const;

    InstallApplicationParams params;
    QStringList applicationsToRestart;

    QFile fileToWrite;
    ConfigHolder* configHolder = nullptr;
};
