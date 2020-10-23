#include "Core/TaskProcessors/AsyncChainProcessor.h"
#include "Core/CommonTasks/AsyncChainTask.h"

AsyncChainProcessor::AsyncChainProcessor(QObject* parent)
    : QObject(parent)
{
}

void AsyncChainProcessor::OnFinished()
{
    tasks.pop_front();

    running = false;
    StartNextTask();
}

void AsyncChainProcessor::AddTask(std::unique_ptr<BaseTask>&& task, Notifier notifier)
{
    Q_ASSERT(task->GetTaskType() == BaseTask::ASYNC_CHAIN);
    AsyncChainTask* chainTask = static_cast<AsyncChainTask*>(task.get());
    notifier.SetProgressDelimiter(chainTask->GetSubtasksCount());
    connect(chainTask, &AsyncChainTask::Finished, this, &AsyncChainProcessor::OnFinished);

    tasks.emplace_back(std::move(task), notifier);

    StartNextTask();
}

void AsyncChainProcessor::StartNextTask()
{
    if (running)
    {
        return;
    }
    if (tasks.empty() == false)
    {
        running = true;

        AsyncTaskParams& params = tasks.front();
        params.notifier.NotifyStarted(params.task.get());
        params.notifier.NotifyProgress(params.task.get(), 0);
        params.task->Run();
    }
}

void AsyncChainProcessor::Terminate()
{
    //do nothing for now
}

std::size_t AsyncChainProcessor::GetTasksCount() const
{
    return tasks.size();
}

AsyncTaskParams::AsyncTaskParams(std::unique_ptr<BaseTask>&& task_, Notifier notifier_)
    : task(static_cast<AsyncChainTask*>(task_.release()))
    , notifier(notifier_)
{
    connect(task.get(), &AsyncChainTask::ChildStarted, this, &AsyncTaskParams::OnStarted);
    connect(task.get(), &AsyncChainTask::ChildProgress, this, &AsyncTaskParams::OnProgress);
    connect(task.get(), &AsyncChainTask::ChildFinished, this, &AsyncTaskParams::OnFinished);
}

AsyncTaskParams::~AsyncTaskParams()
{
    notifier.NotifyFinished(task.get());
}

void AsyncTaskParams::OnStarted(const BaseTask* task)
{
    notifier.NotifyStarted(task);
}

void AsyncTaskParams::OnProgress(const BaseTask* task, quint32 progress)
{
    notifier.NotifyProgress(task, progress);
}

void AsyncTaskParams::OnFinished(const BaseTask* task)
{
    notifier.NotifyFinished(task);
}
