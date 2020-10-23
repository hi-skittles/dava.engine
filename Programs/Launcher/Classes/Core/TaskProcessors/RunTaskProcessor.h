#pragma once

#include "Core/TaskProcessors/BaseTaskProcessor.h"

class RunTaskProcessor final : public BaseTaskProcessor
{
    void AddTask(std::unique_ptr<BaseTask>&& task, Notifier notifier) override;
    void Terminate() override;
    std::size_t GetTasksCount() const override;
};
