#include "Core/CommonTasks/UnzipTask.h"
#include <QObject>

UnzipTask::UnzipTask(ApplicationContext* appContext, const QString& archivePath, const QString& outputPath)
    : BaseTask(appContext)
    , archivePath(archivePath)
    , outputPath(outputPath)
{
}

QString UnzipTask::GetDescription() const
{
    return QObject::tr("Unpacking archive");
}

BaseTask::eTaskType UnzipTask::GetTaskType() const
{
    return ZIP_TASK;
}

QString UnzipTask::GetArchivePath() const
{
    return archivePath;
}

QString UnzipTask::GetOutputPath() const
{
    return outputPath;
}
