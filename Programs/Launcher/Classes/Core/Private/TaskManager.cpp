#include "Core/TaskManager.h"
#include "Core/TaskProcessors/RunTaskProcessor.h"
#include "Core/TaskProcessors/NetworkTaskProcessor.h"
#include "Core/TaskProcessors/ZipTaskProcessor.h"
#include "Core/TaskProcessors/AsyncChainProcessor.h"

TaskManager::TaskManager(QObject* parent)
    : QObject(parent)
{
    taskProcessors[BaseTask::RUN_TASK] = std::make_unique<RunTaskProcessor>();
    taskProcessors[BaseTask::DOWNLOAD_TASK] = std::make_unique<NetworkTaskProcessor>(this);
    taskProcessors[BaseTask::ZIP_TASK] = std::make_unique<ZipTaskProcessor>(this);
    taskProcessors[BaseTask::ASYNC_CHAIN] = std::make_unique<AsyncChainProcessor>(this);
}

TaskManager::~TaskManager() = default;

void TaskManager::AddTask(std::unique_ptr<BaseTask>&& task, const Notifier& notifier)
{
    taskProcessors[task->GetTaskType()]->AddTask(std::move(task), notifier);
}

std::size_t TaskManager::GetTasksCount() const
{
    std::size_t totalCount = 0;
    for (const auto& taskProcessor : taskProcessors)
    {
        totalCount += taskProcessor.second->GetTasksCount();
    }
    return totalCount;
}

void TaskManager::Terminate()
{
    for (auto& taskProcessor : taskProcessors)
    {
        taskProcessor.second->Terminate();
    }
}
