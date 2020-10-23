#pragma once

#include "Core/CommonTasks/BaseTask.h"

class UnzipTask final : public BaseTask
{
public:
    UnzipTask(ApplicationContext* appContext, const QString& archivePath, const QString& outputPath);

    QString GetDescription() const override;
    eTaskType GetTaskType() const override;

    QString GetArchivePath() const;
    QString GetOutputPath() const;

private:
    QString archivePath;
    QString outputPath;
};
