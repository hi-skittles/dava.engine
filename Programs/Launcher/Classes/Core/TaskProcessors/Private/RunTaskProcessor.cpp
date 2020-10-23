#include "Core/TaskProcessors/RunTaskProcessor.h"
#include "Core/CommonTasks/BaseTask.h"
#include "Core/Receiver.h"

void RunTaskProcessor::AddTask(std::unique_ptr<BaseTask>&& task, Notifier notifier)
{
    Q_ASSERT(task->GetTaskType() == BaseTask::RUN_TASK);
    RunTask* runTask = static_cast<RunTask*>(task.get());
    notifier.NotifyStarted(runTask);
    runTask->Run();
    notifier.NotifyFinished(runTask);
}

void RunTaskProcessor::Terminate()
{
    //run task is sync-only, so they can not be terminated
}

std::size_t RunTaskProcessor::GetTasksCount() const
{
    return 0;
}
