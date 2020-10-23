#pragma once

#include "Core/TaskProcessors/BaseTaskProcessor.h"

#include <QObject>
#include <QNetworkAccessManager>

#include <memory>

class DownloadTask;
class QTimer;
class QString;
class QNetworkAccessManager;
class QNetworkReply;

class NetworkTaskProcessor final : public QObject, public BaseTaskProcessor
{
    Q_OBJECT

public:
    NetworkTaskProcessor(QObject* parent);
    ~NetworkTaskProcessor() override;

    void AddTask(std::unique_ptr<BaseTask>&& task, Notifier notifier) override;
    void Terminate() override;
    std::size_t GetTasksCount() const override;

private slots:
    void OnDownloadFinished(QNetworkReply* reply);
    void OnAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible);
    void OnDownloadProgress(qint64 bytes, qint64 total);
    void OnTimer();

private:
    void StartNextTask();
    void TerminateImpl();

    struct TaskParams
    {
        TaskParams(std::unique_ptr<BaseTask>&& task, Notifier notifier);
        ~TaskParams();

        std::unique_ptr<DownloadTask> task;
        Notifier notifier;

        std::list<QNetworkReply*> requests;
    };
    QNetworkAccessManager* networkAccessManager = nullptr;
    QTimer* connectionGuard = nullptr;

    std::list<std::unique_ptr<TaskParams>> tasks;
    std::unique_ptr<TaskParams> currentTask;
};
