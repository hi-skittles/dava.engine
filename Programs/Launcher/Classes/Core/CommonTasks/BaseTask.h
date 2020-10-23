#pragma once

#include <QString>
#include <QVariant>

struct ApplicationContext;

//task data component
class TaskDataHolder
{
public:
    void SetUserData(const QVariant& data);
    const QVariant& GetUserData() const;

private:
    QVariant userData;
};

//task operation result component
class ErrorHolder
{
public:
    //OperationProcessor member functions must work in inherited class const methods
    void SetError(const QString& error) const;
    QString GetError() const;

    bool HasError() const;

private:
    mutable QString errorText;
};

class BaseTask : public TaskDataHolder, public ErrorHolder
{
public:
    BaseTask(ApplicationContext* appManager);

    virtual ~BaseTask() = default;

    //enum to avoid dynamic casts
    enum eTaskType
    {
        RUN_TASK,
        DOWNLOAD_TASK,
        ZIP_TASK,
        ASYNC_CHAIN
    };

    virtual QString GetDescription() const = 0;
    virtual eTaskType GetTaskType() const = 0;

protected:
    ApplicationContext* appContext = nullptr;

private:
    friend class Notifier;

    BaseTask(const BaseTask& task) = delete;
    BaseTask(BaseTask&& task) = delete;
    BaseTask& operator=(const BaseTask& task) = delete;
    BaseTask& operator=(BaseTask&& task) = delete;
};

//base class for run tasks without progress bar
class RunTask : public BaseTask
{
public:
    virtual void Run() = 0;
    eTaskType GetTaskType() const override;

protected:
    RunTask(ApplicationContext* appManager);
};
