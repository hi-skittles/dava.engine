#pragma once

#include "Core/ConsoleTasks/ConsoleBaseTask.h"

#include <memory>

struct ApplicationContext;
struct ConfigHolder;

class InstallAndLaunchTask : public ConsoleBaseTask
{
public:
    InstallAndLaunchTask();
    ~InstallAndLaunchTask() override;

private:
    QCommandLineOption CreateOption() const override;
    void Run(const QStringList& arguments) override;
    void OnUpdateConfigFinished(const QStringList& arguments);

    ApplicationContext* appContext;
    ConfigHolder* configHolder;

    QString appName;
};

Q_DECLARE_METATYPE(InstallAndLaunchTask);
