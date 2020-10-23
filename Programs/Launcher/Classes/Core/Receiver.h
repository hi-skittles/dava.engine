#pragma once

#include <Qt>
#include <vector>
#include <functional>

class BaseTask;

class Receiver
{
public:
    Receiver() = default;

    //say to a user that application is started
    std::function<void(const BaseTask*)> onStarted;

    //display progress to a user
    std::function<void(const BaseTask*, quint32)> onProgress;

    //task finished success or with a errors
    std::function<void(const BaseTask*)> onFinished;
};

class Notifier final
{
public:
    Notifier() = default;
    Notifier(const Receiver& receiver);
    Notifier(const std::vector<Receiver>& receivers);

    //wrappers to avoid iteration over receivers
    void NotifyStarted(const BaseTask* task);
    void NotifyProgress(const BaseTask* task, quint32 progress);
    void NotifyFinished(const BaseTask* task);

    void AddReceiver(const Receiver& receiver);

    void SetProgressDelimiter(int delimiter);

private:
    std::vector<Receiver> receivers;

    int delimiter = 1;
    int step = 0;
    quint32 lastProgress = 0;
};
