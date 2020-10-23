#pragma once

#include "Core/TaskProcessors/BaseTaskProcessor.h"

#include <QObject>

class AsyncChainTask;

class AsyncTaskParams : public QObject
{
    Q_OBJECT

public:
    AsyncTaskParams(std::unique_ptr<BaseTask>&& task, Notifier notifier);
    ~AsyncTaskParams();

    std::unique_ptr<AsyncChainTask> task;
    Notifier notifier;

private slots:
    void OnStarted(const BaseTask* task);
    void OnProgress(const BaseTask* task, quint32 progress);
    void OnFinished(const BaseTask* task);
};

//designed only to manage memory for async chain tasks
class AsyncChainProcessor final : public QObject, public BaseTaskProcessor
{
    Q_OBJECT

public:
    AsyncChainProcessor(QObject* parent);

private slots:
    void OnFinished();

private:
    void AddTask(std::unique_ptr<BaseTask>&& task, Notifier notifier) override;
    void Terminate() override;
    std::size_t GetTasksCount() const override;

    void StartNextTask();

    std::list<AsyncTaskParams> tasks;

    bool running = false;
};
