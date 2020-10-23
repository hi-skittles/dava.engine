#pragma once

#include "Core/Receiver.h"

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QProcess>

class SilentUpdater;
class GuiApplicationManager;
class QTimer;
class QNetworkAccessManager;
class QNetworkReply;
class QJsonObject;

class BAManagerClient : public QObject
{
    Q_OBJECT

public:
    BAManagerClient(GuiApplicationManager* appManager, QObject* parent = nullptr);

    void AskForCommands();

    QString GetProtocolKey() const;
    void SetProtocolKey(const QString& key);

private slots:
    void OnReply(QNetworkReply* reply);
    void OnProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void OnProcessError(QProcess::ProcessError error);

private:
    enum eResult
    {
        //this values is BA-manager ret-code requirements
        SUCCESS = 2,
        FAILURE = 3
    };

    void ProcessCommand(const QJsonObject& object);
    void Post(const QString& urlStr, const QByteArray& data);
    void SendReply(eResult result, const QString& message, const QString& commandID = "0");

    void LaunchProcess(const QJsonObject& requestObj, const QString& commandIDValue);
    void SilentUpdate(const QJsonObject& requestObj, const QString& commandIDValue);

    //taskDelegate
    void OnTaskFinished(const BaseTask* task);

    SilentUpdater* silentUpdater = nullptr;
    GuiApplicationManager* applicationManager = nullptr;
    QTimer* updateTimer = nullptr;
    QNetworkAccessManager* networkManager = nullptr;

    QByteArray protocolKey;
    std::map<QProcess*, QString> startedCommandIDs;

    Receiver receiver;
};
