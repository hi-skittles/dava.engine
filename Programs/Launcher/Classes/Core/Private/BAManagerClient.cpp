#include "Core/BAManagerClient.h"
#include "Core/GuiApplicationManager.h"
#include "Core/TaskManager.h"

#include "Utils/ErrorMessenger.h"
#include "Utils/Utils.h"

#include "Gui/MainWindow.h"

#include "defines.h"

#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include <QProcess>
#include <QDir>

#include <memory>

BAManagerClient::BAManagerClient(GuiApplicationManager* appManager_, QObject* parent /* = nullptr */)
    : QObject(parent)
    , applicationManager(appManager_)
    , updateTimer(new QTimer(this))
    , networkManager(new QNetworkAccessManager(this))
{
    receiver.onFinished = std::bind(&BAManagerClient::OnTaskFinished, this, std::placeholders::_1);

    updateTimer->setSingleShot(false);
    updateTimer->setInterval(90 * 1000);
    connect(updateTimer, &QTimer::timeout, this, &BAManagerClient::AskForCommands);
    connect(networkManager, &QNetworkAccessManager::finished, this, &BAManagerClient::OnReply);
    updateTimer->start();
}

QString BAManagerClient::GetProtocolKey() const
{
    return protocolKey;
}

void BAManagerClient::SetProtocolKey(const QString& key)
{
    protocolKey = key.toUtf8();
}

void BAManagerClient::ProcessCommand(const QJsonObject& object)
{
    QJsonValue commandIDValue = object["command_id"];
    if (commandIDValue.isString() == false)
    {
        QString jsonValueTypeStr = QString::number(static_cast<int>(commandIDValue.type()));
        SendReply(FAILURE, "Can not parse command ID from command, expect a string, got type " + jsonValueTypeStr);
        return;
    }
    QString commandID = commandIDValue.toString();

    QJsonValue requestTypeValue = object["type"];
    if (requestTypeValue.isString() == false)
    {
        QString jsonValueTypeStr = QString::number(static_cast<int>(requestTypeValue.type()));
        SendReply(FAILURE, commandID, "Can not parse type from command, expect a string, got type " + jsonValueTypeStr);
        return;
    }

    QString requestType = requestTypeValue.toString();
    if (requestType == "command")
    {
        LaunchProcess(object, commandID);
    }
    else if (requestType == "install")
    {
        SilentUpdate(object, commandID);
    }
    else
    {
        SendReply(FAILURE, QString("Unknown command: %1").arg(requestType), commandID);
    }
}

void BAManagerClient::SendReply(eResult result, const QString& message, const QString& commandID)
{
    QString urlStr = "http://ba-manager.wargaming.net/modules/jsonAPI/launcher/lite.php";
    QByteArray data = "source=command_result";
    data.append("&command_id=" + commandID);
    data.append("&key=");
    QString key = GetProtocolKey();
    data.append(key.isEmpty() ? "NULL" : key);
    data.append(QString("&version=%1").arg(LAUNCHER_VER));
    data.append("&res_code=");
    data.append(QString::number(static_cast<int>(result)));
    data.append("&message='");
    data.append(message);
    data.append("'");
    Post(urlStr, data);

    if (result != SUCCESS)
    {
        ErrorMessenger::LogMessage(QtWarningMsg, message);
    }
}

void BAManagerClient::AskForCommands()
{
    QString urlStr = "http://ba-manager.wargaming.net/modules/jsonAPI/launcher/lite.php";
    QByteArray data = "source=commands";
    data.append("&key=");
    QString key = GetProtocolKey();
    data.append(key.isEmpty() ? "NULL" : key);
    data.append(QString("&version=%1").arg(LAUNCHER_VER));
    data.append("&os=");
    data.append(platformString);
    Post(urlStr, data);
}

void BAManagerClient::Post(const QString& urlStr, const QByteArray& data)
{
    QNetworkRequest request;
    request.setUrl(QUrl(urlStr));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    networkManager->post(request, data);
}

void BAManagerClient::OnReply(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError)
    {
        ErrorMessenger::LogMessage(QtWarningMsg, "Network error: " + reply->errorString());
        return;
    }

    QJsonParseError parseError;
    QByteArray readedData = reply->readAll();
    QJsonDocument document = QJsonDocument::fromJson(readedData, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        SendReply(FAILURE, "Can not parse JSON reply, error is: " + parseError.errorString());
        return;
    }

    QJsonObject rootObj = document.object();
    QJsonValue keyValue = rootObj["key"];
    if (keyValue.isString())
    {
        SetProtocolKey(keyValue.toString());
    }
    else if (keyValue.isNull() == false)
    {
        QString jsonValueTypeStr = QString::number(static_cast<int>(keyValue.type()));
        SendReply(FAILURE, "Can not parse key from reply, expect a string, got type " + jsonValueTypeStr);
    }
    QJsonValue commandsValue = rootObj["commands"];
    if (commandsValue.isArray() == false && commandsValue.isNull() == false)
    {
        QString jsonValueTypeStr = QString::number(static_cast<int>(commandsValue.type()));
        SendReply(FAILURE, "Can not parse commands from reply, expects an array, got type " + jsonValueTypeStr);
    }
    else
    {
        QJsonArray commands = commandsValue.toArray();
        for (const QJsonValueRef& valueRef : commands)
        {
            if (valueRef.isObject())
            {
                ProcessCommand(valueRef.toObject());
            }
            else
            {
                SendReply(FAILURE, "Got a command, which not an object");
            }
        }
    }
    QJsonValue commandResultValue = rootObj["command_result"];
    if (commandResultValue.isArray() == false && commandResultValue.isNull() == false)
    {
        QString jsonValueTypeStr = QString::number(static_cast<int>(commandResultValue.type()));
        SendReply(FAILURE, "Can not parse command result from reply, expects an array, got type " + jsonValueTypeStr);
    }
    else
    {
        //we have not any requirements to process answer result.
    }
}

void BAManagerClient::OnProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (process == nullptr)
    {
        ErrorMessenger::LogMessage(QtWarningMsg, "Internal error: got Process Finished not from process");
        return;
    }
    process->deleteLater();

    QString commandID = startedCommandIDs[process];
    startedCommandIDs.erase(process);
    if (commandID.isEmpty())
    {
        ErrorMessenger::LogMessage(QtWarningMsg, "Internal error: process is not registered");
        return;
    }
    QByteArray standardOutput = process->readAllStandardOutput();
    QByteArray standardError = process->readAllStandardError();
    eResult result = (exitCode == 0 && exitStatus == QProcess::NormalExit) ? SUCCESS : FAILURE;
    QJsonObject replyObject{
        { "process exit code", QString::number(exitCode) },
        { "process exit status", QString::number(exitStatus) },
        { "standard output", QString(standardOutput) },
        { "standard error", QString(standardError) }
    };
    QJsonDocument document(replyObject);
    SendReply(result, document.toJson(), commandID);
}

void BAManagerClient::OnProcessError(QProcess::ProcessError error)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (process == nullptr)
    {
        ErrorMessenger::LogMessage(QtWarningMsg, "Internal error: got Process Error not from process");
        return;
    }
    process->deleteLater();

    QString commandID = startedCommandIDs[process];
    if (commandID.isEmpty())
    {
        ErrorMessenger::LogMessage(QtWarningMsg, "Internal error: process is not registered");
        return;
    }
    QString reply = QString("Error occured while launching process. Qt error code is %1").arg(static_cast<int>(error));
    SendReply(FAILURE, reply, commandID);
}

void BAManagerClient::LaunchProcess(const QJsonObject& requestObj, const QString& commandID)
{
    QJsonValue commandValue = requestObj["cmd"];
    if (commandValue.isString() == false)
    {
        QString jsonValueTypeStr = QString::number(static_cast<int>(commandValue.type()));
        SendReply(FAILURE, "Can not parse cmd from command, expect a string, got type " + jsonValueTypeStr, commandID);
        return;
    }

    QString cmd = commandValue.toString();
    ErrorMessenger::LogMessage(QtDebugMsg, "launched command from BA-manager: " + cmd);
    QProcess* process = new QProcess();
    connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &BAManagerClient::OnProcessFinished);
    connect(process, &QProcess::errorOccurred, this, &BAManagerClient::OnProcessError);
    startedCommandIDs[process] = commandID;
    QString processString =
#ifdef Q_OS_WIN
    "cmd.exe /c " + cmd;
#else
    cmd;
#endif //platform
    process->start(processString, QProcess::ReadWrite);
}

void BAManagerClient::SilentUpdate(const QJsonObject& requestObj, const QString& commandID)
{
    QJsonValue branchNameValue = requestObj["branchName"];
    if (branchNameValue.isString() == false)
    {
        QString jsonValueTypeStr = QString::number(static_cast<int>(branchNameValue.type()));
        SendReply(FAILURE, "Can not parse branch name from command, expect a string, got type " + jsonValueTypeStr, commandID);
        return;
    }
    QString branchName = branchNameValue.toString();

    QJsonValue appNameValue = requestObj["build_name"];
    if (appNameValue.isString() == false)
    {
        QString jsonValueTypeStr = QString::number(static_cast<int>(appNameValue.type()));
        SendReply(FAILURE, "Can not parse app name from command, expect a string, got type " + jsonValueTypeStr, commandID);
        return;
    }

    QJsonValue forceRestartValue = requestObj["force_start"];
    if (forceRestartValue.isString() == false)
    {
        QString jsonValueTypeStr = QString::number(static_cast<int>(appNameValue.type()));
        SendReply(FAILURE, "Can not parse force_start from command, expect a string, got type " + jsonValueTypeStr, commandID);
        return;
    }
    InstallApplicationParams params;
    params.branch = branchName;
    QString appName = appNameValue.toString();
    params.app = applicationManager->GetString(appName);
    if (forceRestartValue.isBool() && forceRestartValue.toBool())
    {
        params.appToStart = params.app;
    }
    ::FillAppFields(&params.newVersion, requestObj, IsToolset(appName));

    std::unique_ptr<BaseTask> task = applicationManager->GetContext()->CreateTask<InstallApplicationTask>(applicationManager->GetConfigHolder(), params);
    task->SetUserData(commandID);
    applicationManager->AddTaskWithCustomReceivers(std::move(task), { receiver, applicationManager->GetMainWindow()->GetReceiver() });
}

void BAManagerClient::OnTaskFinished(const BaseTask* task)
{
    if (task->GetTaskType() != BaseTask::ASYNC_CHAIN)
    {
        return;
    }

    const QVariant& data = task->GetUserData();
    if (data.canConvert<QString>())
    {
        QString commandId = data.toString();
        const QString& error = task->GetError();
        if (error.isEmpty())
        {
            SendReply(SUCCESS, "success", commandId);
        }
        else
        {
            SendReply(FAILURE, error, commandId);
        }
    }
}
