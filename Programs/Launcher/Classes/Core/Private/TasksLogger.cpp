#include "Core/TasksLogger.h"
#include "Core/CommonTasks/BaseTask.h"

#include "Utils/ErrorMessenger.h"

#include <QDateTime>

TasksLogger::TasksLogger()
{
    using namespace std::placeholders;
    receiver.onStarted = std::bind(&TasksLogger::OnTaskStarted, this, _1);
    receiver.onFinished = std::bind(&TasksLogger::OnTaskFinished, this, _1);
}

Receiver TasksLogger::GetReceiver() const
{
    return receiver;
}

void TasksLogger::OnTaskStarted(const BaseTask* task)
{
    ErrorMessenger::LogMessage(QtDebugMsg, "started: " + task->GetDescription());
}

void TasksLogger::OnTaskFinished(const BaseTask* task)
{
    if (task->HasError())
    {
        ErrorMessenger::LogMessage(QtWarningMsg, "error occurred: " + task->GetError() + " on task: " + task->GetDescription());
    }
    ErrorMessenger::LogMessage(QtDebugMsg, "finished: " + task->GetDescription());
}
