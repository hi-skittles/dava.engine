#include "Core/CommonTasks/UpdateConfigTask.h"
#include "Core/CommonTasks/DownloadTask.h"
#include "Core/ApplicationContext.h"
#include "Core/ConfigHolder.h"

#include "Gui/MainWindow.h"

#include <QBuffer>
#include <memory>

UpdateConfigTask::UpdateConfigTask(ApplicationContext* appContext, ConfigHolder* configHolder_, const std::vector<QUrl>& urls)
    : AsyncChainTask(appContext)
    , configHolder(configHolder_)
{
    for (const QUrl& url : urls)
    {
        QBuffer* buffer = new QBuffer(this);
        buffer->open(QIODevice::ReadWrite);
        buffers[url] = buffer;
    }
}

QString UpdateConfigTask::GetDescription() const
{
    return QObject::tr("Updating configuration");
}

void UpdateConfigTask::Run()
{
    configHolder->remoteConfig.Clear();
    QString description = QObject::tr("Downloading configuration");
    std::unique_ptr<BaseTask> task = appContext->CreateTask<DownloadTask>(description, buffers);
    Q_ASSERT(task != nullptr);

    Receiver receiver;
    receiver.onFinished = [this](const BaseTask* task) {
        if (task->HasError())
        {
            emit Finished();
        }
        else
        {
            OnLoaded();
        }
    };
    appContext->taskManager.AddTask(std::move(task), Notifier({ transparentReceiver, receiver }));
}

void UpdateConfigTask::OnLoaded()
{
    for (auto& bufferItem : buffers)
    {
        QBuffer* buffer = static_cast<QBuffer*>(bufferItem.second);
        Q_ASSERT(buffer->size() > 0);
        if (configHolder->remoteConfig.ParseJSON(buffer->data(), this))
        {
            QString webPageUrl = configHolder->remoteConfig.GetWebpageURL();
            if (webPageUrl.isEmpty() == false)
            {
                configHolder->localConfig.SetWebpageURL(webPageUrl);
            }
            configHolder->localConfig.CopyStringsAndFavsFromConfig(&configHolder->remoteConfig);
        }
    }
    configHolder->remoteConfig.UpdateApplicationsNames();
    configHolder->localConfig.UpdateApplicationsNames();
    emit Finished();
}
