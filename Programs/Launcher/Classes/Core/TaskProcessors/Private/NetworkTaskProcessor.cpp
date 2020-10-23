#include "Core/TaskProcessors/NetworkTaskProcessor.h"
#include "Core/CommonTasks/DownloadTask.h"
#include "Core/Receiver.h"

#include <QNetworkReply>
#include <QTimer>

NetworkTaskProcessor::TaskParams::TaskParams(std::unique_ptr<BaseTask>&& task_, Notifier notifier_)
    : task(static_cast<DownloadTask*>(task_.release()))
    , notifier(notifier_)
{
}

NetworkTaskProcessor::TaskParams::~TaskParams()
{
    Q_ASSERT(requests.empty());
}

NetworkTaskProcessor::NetworkTaskProcessor(QObject* parent)
    : QObject(parent)
    , networkAccessManager(new QNetworkAccessManager(this))
    , connectionGuard(new QTimer(this))
{
    connect(networkAccessManager, &QNetworkAccessManager::finished, this, &NetworkTaskProcessor::OnDownloadFinished);
    connect(networkAccessManager, &QNetworkAccessManager::networkAccessibleChanged, this, &NetworkTaskProcessor::OnAccessibleChanged);

    connectionGuard->setSingleShot(true);
    connectionGuard->setInterval(1 * 60 * 1000);
    connect(connectionGuard, &QTimer::timeout, this, &NetworkTaskProcessor::OnTimer);
}

NetworkTaskProcessor::~NetworkTaskProcessor() = default;

void NetworkTaskProcessor::AddTask(std::unique_ptr<BaseTask>&& task, Notifier notifier)
{
    Q_ASSERT(task->GetTaskType() == BaseTask::DOWNLOAD_TASK);
    Q_ASSERT(task != nullptr);
    tasks.emplace_back(new TaskParams(std::move(task), notifier));
    StartNextTask();
}

void NetworkTaskProcessor::Terminate()
{
    if (currentTask == nullptr)
    {
        return;
    }
    currentTask->task->SetCancelled(true);
    TerminateImpl();
}

void NetworkTaskProcessor::TerminateImpl()
{
    std::list<QNetworkReply*> requests = currentTask->requests;
    for (QNetworkReply* reply : requests)
    {
        reply->abort();
    }
}

void NetworkTaskProcessor::StartNextTask()
{
    if (tasks.empty() || currentTask != nullptr)
    {
        return;
    }

    currentTask = std::move(tasks.front());
    tasks.pop_front();

    currentTask->notifier.NotifyStarted(currentTask->task.get());

//on mac os x when connection fails once QNetworkAccessManager aborting each next  connection
#ifdef Q_OS_MAC
    if (networkAccessManager->networkAccessible() != QNetworkAccessManager::Accessible)
    {
        networkAccessManager->setNetworkAccessible(QNetworkAccessManager::Accessible);
    }
#endif //Q_OS_MAC

    std::vector<QUrl> urls = currentTask->task->GetUrls();
    Q_ASSERT(urls.empty() == false);

    for (const QUrl& url : urls)
    {
        QNetworkReply* reply = networkAccessManager->get(QNetworkRequest(url));
        Q_ASSERT(reply != nullptr);
        connect(reply, &QNetworkReply::downloadProgress, this, &NetworkTaskProcessor::OnDownloadProgress);

        //in some cases currentTask is empty
        //as an example QNetworkReply can call finished() signal immidiately
        if (currentTask != nullptr)
        {
            currentTask->requests.push_back(reply);
        }
    }
    if (currentTask != nullptr)
    {
        connectionGuard->start();
    }
}

void NetworkTaskProcessor::OnDownloadFinished(QNetworkReply* reply)
{
    //on Windows we can receive this callback even if current task is empty
    if (currentTask == nullptr)
    {
        return;
    }
    Q_ASSERT(currentTask->task->GetTaskType() == BaseTask::DOWNLOAD_TASK);
    reply->deleteLater();

    currentTask->requests.remove(reply);
    if (reply->error() != QNetworkReply::NoError)
    {
        if (currentTask->task->HasError() == false)
        {
            currentTask->task->SetError(QObject::tr("Network error: %1").arg(reply->errorString()));
        }
        //we can receive error only for the some replies, and all other replies will hang
        if (reply->error() != QNetworkReply::OperationCanceledError && currentTask->requests.empty() == false)
        {
            Terminate();
            return;
        }
    }
    if (currentTask->requests.empty())
    {
        connectionGuard->stop();
        currentTask->notifier.NotifyFinished(currentTask->task.get());
        currentTask = nullptr;
        StartNextTask();
    }
}

void NetworkTaskProcessor::OnAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible)
{
    if (accessible == QNetworkAccessManager::NotAccessible)
    {
        if (currentTask != nullptr)
        {
            currentTask->task->SetError(QObject::tr("Network is unaccessible"));
            //on the os x after networkAccessibleChanged QNetworkReply may not send finished() signal
            Terminate();
        }
    }
}

void NetworkTaskProcessor::OnDownloadProgress(qint64 bytes, qint64 total)
{
    //on connection abort we have an empty currentTask and downloadProgress with arguments 0, 0
    if (currentTask == nullptr || total == -1)
    {
        return;
    }
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(reply != nullptr);
    currentTask->task->AddLoadedData(reply->url(), reply->readAll());

    connectionGuard->start();

    size_t size = currentTask->task->GetUrls().size();
    Q_ASSERT(size > 0);
    float multiplier = (100.0f * (size - currentTask->requests.size() + 1)) / size;
    int progress = ((static_cast<float>(bytes)) / total) * multiplier;
    currentTask->notifier.NotifyProgress(currentTask->task.get(), progress);
}

void NetworkTaskProcessor::OnTimer()
{
    if (currentTask != nullptr)
    {
        currentTask->task->SetError("Operation cancelled by timeout");
        TerminateImpl();
    }
}

std::size_t NetworkTaskProcessor::GetTasksCount() const
{
    return tasks.size() + (currentTask != nullptr ? 1 : 0);
}
