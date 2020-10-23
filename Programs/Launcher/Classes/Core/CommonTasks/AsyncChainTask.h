#pragma once

#include "Core/CommonTasks/BaseTask.h"
#include "Core/Receiver.h"

#include <QObject>

#include <functional>

class AsyncChainTask : public QObject, public BaseTask
{
    Q_OBJECT

public:
    AsyncChainTask(ApplicationContext* appContext);

    virtual void Run() = 0;
    virtual int GetSubtasksCount() const;

    //this signal is used only by AsyncChainTaskProcessor
    //later AsyncChainProcessor and AsyncChainTask must be replaced by std::function
    //it must be done when launcher will be able to start multiple tasks at once
signals:
    void Finished() const;

    void ChildStarted(const BaseTask* task);
    void ChildProgress(const BaseTask* task, quint32 progress);
    void ChildFinished(const BaseTask* task);

protected:
    //this receiver resend data to users
    Receiver transparentReceiver;

private:
    eTaskType GetTaskType() const override;
};
