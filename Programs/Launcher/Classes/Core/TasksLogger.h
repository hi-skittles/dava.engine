#pragma once

#include "Core/Receiver.h"

class TasksLogger final
{
public:
    TasksLogger();

    Receiver GetReceiver() const;

private:
    void OnTaskStarted(const BaseTask* task);
    void OnTaskFinished(const BaseTask* task);

    Receiver receiver;
};
