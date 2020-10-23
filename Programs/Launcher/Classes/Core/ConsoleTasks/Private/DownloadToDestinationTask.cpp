#include "Core/ConsoleTasks/DownloadToDestinationTask.h"
#include "Core/ConsoleTasks/ConsoleTasksCollection.h"
#include "Core/CommonTasks/LoadLocalConfigTask.h"
#include "Core/CommonTasks/UpdateConfigTask.h"
#include "Core/CommonTasks/DownloadTask.h"
#include "Core/CommonTasks/UnzipTask.h"
#include "Core/ApplicationContext.h"
#include "Core/ConfigHolder.h"
#include "Core/UrlsHolder.h"

#include "Utils/Utils.h"

#include <QDir>
#include <QDebug>

#include <iostream>

REGISTER_CLASS(DownloadToDestinationTask);

DownloadToDestinationTask::DownloadToDestinationTask()
    : appContext(new ApplicationContext())
    , configHolder(new ConfigHolder())
{
}

DownloadToDestinationTask::~DownloadToDestinationTask()
{
    delete appContext;
    delete configHolder;
}

QCommandLineOption DownloadToDestinationTask::CreateOption() const
{
    return QCommandLineOption(QStringList() << "d"
                                            << "download",
                              QObject::tr("download file to <destination> from <application> <branch> <version>;\n"
                                          "if no <branch> is set - will be used Stable\n"
                                          "if no <version> is set - will be used most recent version\n"
                                          "example: -d c:/temp QuickEd 3444/from 4.4.0_2017-09-06_20-00-24_4296\n"
                                          "another example: -d c:/temp QuickEd 3444 4296"));
}

void DownloadToDestinationTask::Run(const QStringList& arguments)
{
    if (arguments.isEmpty())
    {
        qDebug() << "error: pass destination directory, please";
        exit(1);
    }

    destPath = arguments.at(0);
    QFileInfo fi(destPath);
    if (fi.exists() == false)
    {
        QDir dir(destPath);
        if (dir.mkpath(".") == false)
        {
            qDebug() << "error: can not create dir" << destPath;
            exit(1);
        }
    }
    else if (fi.isDir() == false)
    {
        qDebug() << "error: destination is not a directory" << destPath;
        exit(1);
    }

    Receiver receiver;
    receiver.onFinished = [](const BaseTask* task) {
        if (task->HasError())
        {
            qDebug() << "error: " + task->GetError();
            exit(1);
        }
    };

    std::unique_ptr<BaseTask> loadConfigTask = appContext->CreateTask<LoadLocalConfigTask>(configHolder, FileManager::GetLocalConfigFilePath());
    appContext->taskManager.AddTask(std::move(loadConfigTask), receiver);

    receiver.onStarted = [](const BaseTask* task) {
        qDebug() << task->GetDescription();
    };
    receiver.onProgress = [](const BaseTask* task, quint32 progress) {
        std::cout << "progress: " << progress << "\r";
    };
    receiver.onFinished = [arguments, this](const BaseTask* task) {
        if (task->HasError())
        {
            qDebug() << "error: " + task->GetError();
            exit(1);
        }
        else if (dynamic_cast<const UpdateConfigTask*>(task) != nullptr)
        {
            OnUpdateConfigFinished(arguments);
        }
    };

    std::unique_ptr<BaseTask> updateTask = appContext->CreateTask<UpdateConfigTask>(configHolder, appContext->urlsHolder.GetURLs());
    appContext->taskManager.AddTask(std::move(updateTask), receiver);
}

void DownloadToDestinationTask::OnUpdateConfigFinished(const QStringList& arguments)
{
    if (arguments.size() < 2)
    {
        qDebug() << "error: pass application name, please";
        exit(1);
    }

    QString applicationName = arguments.at(1);

    QString branchName = "Stable";
    if (arguments.size() >= 3)
    {
        branchName = arguments.at(2);
    }

    QString versionName = "recent";
    if (arguments.size() >= 4)
    {
        versionName = arguments.at(3);
    }

    AppVersion* version = LauncherUtils::FindVersion(&configHolder->remoteConfig, branchName, applicationName, versionName);
    if (version == nullptr)
    {
        qDebug() << "error: version " << versionName << "for application" << applicationName << "in branch" << branchName << "not found";
        exit(1);
    }

    QString fileName = FileManager::GetFileNameFromURL(version->url);
    archivePath = destPath + "/" + fileName;
    QFile* file = new QFile(archivePath);
    if (file->open(QFile::Truncate | QFile::WriteOnly) == false)
    {
        qDebug() << "error: can not open file" << destPath;
        exit(1);
    }

    Receiver receiver;
    receiver.onStarted = [applicationName](const BaseTask* task) {
        qDebug() << "Start loading " + applicationName;
    };
    receiver.onProgress = [](const BaseTask* task, quint32 progress) {
        std::cout << "progress: " << progress << "\r";
    };
    receiver.onFinished = [file, fileName, arguments, this](const BaseTask* task) {
        file->close();
        delete file;

        if (task->HasError())
        {
            qDebug() << "error: " + task->GetError();
            exit(1);
        }
        else
        {
            qDebug() << "loaded file with name:" << fileName;
            OnDownloadFinished(arguments);
        }
    };

    std::unique_ptr<BaseTask> downloadTask = appContext->CreateTask<DownloadTask>("loading application " + applicationName, version->url, file);
    appContext->taskManager.AddTask(std::move(downloadTask), receiver);
}

void DownloadToDestinationTask::OnDownloadFinished(const QStringList& arguments)
{
    Receiver receiver;
    receiver.onStarted = [](const BaseTask* task) {
        qDebug() << "unpacking";
    };
    receiver.onProgress = [](const BaseTask* task, quint32 progress) {
        std::cout << "progress: " << progress << "\r";
    };

    receiver.onFinished = [this](const BaseTask* task) {
        if (QFile::remove(archivePath) == false)
        {
            qDebug() << "can not remove tmp file" << archivePath;
        }
        if (task->HasError())
        {
            qDebug() << "error: " + task->GetError();
            exit(1);
        }
        else
        {
            qDebug() << "task finished!";
            exit(0);
        }
    };

    std::unique_ptr<BaseTask> task = appContext->CreateTask<UnzipTask>(archivePath, destPath);
    appContext->taskManager.AddTask(std::move(task), receiver);
}
