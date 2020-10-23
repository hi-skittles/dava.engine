#pragma once

#include "Core/CommonTasks/BaseTask.h"
#include "Core/Receiver.h"

#include <QObject>

#include <memory>

class BaseTaskProcessor;

class TaskManager : public QObject
{
    Q_OBJECT

public:
    TaskManager(QObject* parent = nullptr);
    ~TaskManager();

    void AddTask(std::unique_ptr<BaseTask>&& task, const Notifier& notifier);
    std::size_t GetTasksCount() const;

public slots:
    void Terminate();

private:
    std::map<BaseTask::eTaskType, std::unique_ptr<BaseTaskProcessor>> taskProcessors;
};
