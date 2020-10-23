#include "Core/Receiver.h"
#include "Core/CommonTasks/BaseTask.h"

Notifier::Notifier(const Receiver& receiver)
    : receivers(1, receiver)
{
}

Notifier::Notifier(const std::vector<Receiver>& receivers_)
    : receivers(receivers_)
{
}

void Notifier::NotifyStarted(const BaseTask* task)
{
    for (Receiver& receiver : receivers)
    {
        if (receiver.onStarted)
        {
            receiver.onStarted(task);
        }
    }
}

void Notifier::NotifyProgress(const BaseTask* task, quint32 progress)
{
    if (progress < lastProgress)
    {
        step++;
    }
    lastProgress = progress;

    for (Receiver& receiver : receivers)
    {
        if (receiver.onProgress)
        {
            receiver.onProgress(task, (step * 100 + progress) / delimiter);
        }
    }
}

void Notifier::NotifyFinished(const BaseTask* task)
{
    //first notify receiver that task if finished
    //receiver can not produce another tasks on finished
    //if you need to produce one - create AsyncChainTask
    for (Receiver& receiver : receivers)
    {
        if (receiver.onFinished)
        {
            receiver.onFinished(task);
        }
    }
}

void Notifier::AddReceiver(const Receiver& receiver)
{
    receivers.push_back(receiver);
}

void Notifier::SetProgressDelimiter(int delimiter_)
{
    Q_ASSERT(delimiter_ > 0);
    delimiter = delimiter_;
}
