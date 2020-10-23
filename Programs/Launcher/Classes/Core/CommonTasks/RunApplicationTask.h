#pragma once

#include "Core/CommonTasks/BaseTask.h"

struct ConfigHolder;

class RunApplicationTask : public RunTask
{
public:
    RunApplicationTask(ApplicationContext* appContext, ConfigHolder* configHolder, const QString& branch, const QString& app, const QString& version);

private:
    QString GetDescription() const override;
    void Run() override;

    QString branch;
    QString app;
    QString version;
    ConfigHolder* configHolder = nullptr;
};
