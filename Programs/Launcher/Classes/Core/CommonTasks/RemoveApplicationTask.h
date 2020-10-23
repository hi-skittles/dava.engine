#pragma once

#include "Core/CommonTasks/BaseTask.h"
#include "Data/ConfigParser.h"

struct ConfigHolder;

class RemoveApplicationTask : public RunTask
{
public:
    RemoveApplicationTask(ApplicationContext* appContext, ConfigHolder* configHolder, const QString& branch, const QString& app);

private:
    QString GetDescription() const override;
    void Run() override;

    bool TryStopApp(const QString& runPath) const;
    bool CanRemoveApp(const QString& branchID, const QString& appID, AppVersion* localVersion) const;
    bool RemoveApplicationImpl(const QString& branchID, const QString& appID, AppVersion* localVersion);

    QString branch;
    QString app;

    ConfigHolder* configHolder = nullptr;
};
