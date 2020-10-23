#pragma once

#include "Core/CommonTasks/BaseTask.h"

struct ConfigHolder;

class LoadLocalConfigTask : public RunTask
{
public:
    LoadLocalConfigTask(ApplicationContext* appContext, ConfigHolder* configHolder, const QString& localConfigPath);

private:
    QString GetDescription() const override;

    void Run() override;

    QString localConfigPath;
    ConfigHolder* configHolder;
};
