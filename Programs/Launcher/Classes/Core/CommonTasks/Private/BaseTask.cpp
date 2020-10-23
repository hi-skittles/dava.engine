#include "Core/CommonTasks/BaseTask.h"
#include "Core/ApplicationContext.h"

void TaskDataHolder::SetUserData(const QVariant& data)
{
    userData = data;
}

const QVariant& TaskDataHolder::GetUserData() const
{
    return userData;
}

void ErrorHolder::SetError(const QString& error) const
{
    errorText = error;
}

QString ErrorHolder::GetError() const
{
    return errorText;
}

bool ErrorHolder::HasError() const
{
    return errorText.isEmpty() == false;
}

BaseTask::BaseTask(ApplicationContext* appContext_)
    : appContext(appContext_)
{
}

RunTask::RunTask(ApplicationContext* appContext)
    : BaseTask(appContext)
{
}

BaseTask::eTaskType RunTask::GetTaskType() const
{
    return BaseTask::RUN_TASK;
}
