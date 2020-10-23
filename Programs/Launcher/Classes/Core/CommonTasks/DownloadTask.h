#pragma once

#include "Core/CommonTasks/BaseTask.h"

#include <QString>
#include <QUrl>
#include <QByteArray>

class QIODevice;

class DownloadTask final : public BaseTask
{
public:
    DownloadTask(ApplicationContext* appContext, const QString& description, const std::map<QUrl, QIODevice*>& buffers);
    DownloadTask(ApplicationContext* appContext, const QString& description, const QUrl& url, QIODevice* writeBuffer);

    QString GetDescription() const override;
    eTaskType GetTaskType() const override;

    void AddLoadedData(const QUrl& url, const QByteArray& data);

    std::vector<QUrl> GetUrls() const;

    bool IsCancelled() const;
    void SetCancelled(bool cancelled);

protected:
    QString description;
    std::map<QUrl, QIODevice*> buffers;

    bool isCancelled = false;
};
