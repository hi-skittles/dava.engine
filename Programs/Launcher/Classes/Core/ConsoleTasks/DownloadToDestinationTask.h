#pragma once

#include "Core/ConsoleTasks/ConsoleBaseTask.h"

struct ApplicationContext;
struct ConfigHolder;

class DownloadToDestinationTask : public ConsoleBaseTask
{
public:
    DownloadToDestinationTask();
    ~DownloadToDestinationTask() override;

private:
    QCommandLineOption CreateOption() const override;
    void Run(const QStringList& arguments) override;

    void OnUpdateConfigFinished(const QStringList& arguments);
    void OnDownloadFinished(const QStringList& arguments);

    ApplicationContext* appContext = nullptr;
    ConfigHolder* configHolder = nullptr;
    QString destPath;
    QString archivePath;
};

Q_DECLARE_METATYPE(DownloadToDestinationTask);
