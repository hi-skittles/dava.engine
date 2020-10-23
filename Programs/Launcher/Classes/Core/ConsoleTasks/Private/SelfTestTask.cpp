#include "Core/ConsoleTasks/SelfTestTask.h"
#include "Core/ConsoleTasks/ConsoleTasksCollection.h"

#include "Core/ApplicationContext.h"
#include "Core/ConfigHolder.h"

#include "Core/CommonTasks/LoadLocalConfigTask.h"
#include "Core/CommonTasks/UpdateConfigTask.h"

#include "Gui/PreferencesDialog.h"

#include <QDebug>

#include <iostream>

REGISTER_CLASS(SelfTestTask);

namespace SelfTestTaskDetails
{
void PrintLine(const std::string& text)
{
    std::cout << text << std::endl
              << std::flush;
}

void TeamcityOutput(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    QString status;
    switch (type)
    {
    case QtCriticalMsg:
    case QtFatalMsg:
        status = "ERROR";
        break;
    case QtWarningMsg:
        status = "WARNING";
        break;
    default:
        status = "NORMAL";
        break;
    }

    QString output = "##teamcity[message text=\'" + msg + "\' errorDetails=\'\' status=\'" + status + "\']\n";

    std::cout << output.toStdString() << std::endl;
}
}

SelfTestTask::SelfTestTask()
    : appContext(new ApplicationContext())
    , configHolder(new ConfigHolder())
{
}

SelfTestTask::~SelfTestTask()
{
    delete appContext;
    delete configHolder;
}

QCommandLineOption SelfTestTask::CreateOption() const
{
    return QCommandLineOption(QStringList() << "s"
                                            << "selftest",
                              QObject::tr("starts the self-test process"));
}

void SelfTestTask::Run(const QStringList& arguments)
{
    qInstallMessageHandler(SelfTestTaskDetails::TeamcityOutput);
    PreferencesDialog::LoadPreferences(appContext);

    Receiver receiver;
    receiver.onStarted = [](const BaseTask* task) {
        qDebug() << task->GetDescription();
    };
    receiver.onFinished = [](const BaseTask* task) {
        if (task->HasError())
        {
            qCritical() << task->GetDescription() + ": " + task->GetError();
            exit(1);
        }
    };

    std::unique_ptr<BaseTask> loadConfigTask = appContext->CreateTask<LoadLocalConfigTask>(configHolder, FileManager::GetLocalConfigFilePath());
    appContext->taskManager.AddTask(std::move(loadConfigTask), receiver);

    receiver.onStarted = [](const BaseTask* task) {
        qDebug() << task->GetDescription();
    };
    receiver.onFinished = [arguments, this](const BaseTask* task) {
        if (task->HasError())
        {
            qCritical() << task->GetDescription() + ": " + task->GetError();
            exit(1);
        }
        else if (dynamic_cast<const UpdateConfigTask*>(task) != nullptr)
        {
            qDebug() << "test succesfully finished";
            exit(0);
        }
    };
    std::unique_ptr<BaseTask> updateTask = appContext->CreateTask<UpdateConfigTask>(configHolder, appContext->urlsHolder.GetURLs());
    appContext->taskManager.AddTask(std::move(updateTask), receiver);
}
