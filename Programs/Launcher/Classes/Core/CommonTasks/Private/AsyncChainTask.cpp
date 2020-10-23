#include "Core/CommonTasks/AsyncChainTask.h"

#include <functional>

AsyncChainTask::AsyncChainTask(ApplicationContext* appContext)
    : BaseTask(appContext)
{
    transparentReceiver.onStarted = std::bind(&AsyncChainTask::ChildStarted, this, std::placeholders::_1);
    transparentReceiver.onProgress = std::bind(&AsyncChainTask::ChildProgress, this, std::placeholders::_1, std::placeholders::_2);
    transparentReceiver.onFinished = std::bind(&AsyncChainTask::ChildFinished, this, std::placeholders::_1);
}

int AsyncChainTask::GetSubtasksCount() const
{
    return 1;
}

BaseTask::eTaskType AsyncChainTask::GetTaskType() const
{
    return ASYNC_CHAIN;
}
