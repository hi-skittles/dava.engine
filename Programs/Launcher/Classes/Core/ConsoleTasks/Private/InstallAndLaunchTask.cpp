#include "Core/ConsoleTasks/InstallAndLaunchTask.h"
#include "Core/ConsoleTasks/ConsoleTasksCollection.h"

#include "Core/CommonTasks/LoadLocalConfigTask.h"
#include "Core/CommonTasks/UpdateConfigTask.h"
#include "Core/CommonTasks/RunApplicationTask.h"
#include "Core/CommonTasks/InstallApplicationTask.h"
#include "Core/ApplicationContext.h"
#include "Core/ConfigHolder.h"
#include "Core/UrlsHolder.h"

#include "Gui/PreferencesDialog.h"

#include "Utils/Utils.h"

#include <QDebug>

#include <iostream>

REGISTER_CLASS(InstallAndLaunchTask);

InstallAndLaunchTask::InstallAndLaunchTask()
    : appContext(new ApplicationContext())
    , configHolder(new ConfigHolder())
{
}

InstallAndLaunchTask::~InstallAndLaunchTask()
{
    delete appContext;
    delete configHolder;
}

QCommandLineOption InstallAndLaunchTask::CreateOption() const
{
    return QCommandLineOption(QStringList() << "i"
                                            << "install",
                              QObject::tr("install <application> <branch> <version> and launch it;\n"
                                          "if no <version> is set - will be used most recent version\n"
                                          "if no <branch> is set - will be used Stable\n"
                                          "example -i QuickEd Stable 4389"));
}

void InstallAndLaunchTask::Run(const QStringList& arguments)
{
    PreferencesDialog::LoadPreferences(appContext);

    if (arguments.isEmpty())
    {
        qDebug() << "error: pass application, please";
        exit(1);
    }

    appName = arguments.at(0);

    Receiver loadConfigReceiver;
    loadConfigReceiver.onFinished = [](const BaseTask* task) {
        if (task->HasError())
        {
            qDebug() << "error: " + task->GetError();
            exit(1);
        }
    };

    std::unique_ptr<BaseTask> loadConfigTask = appContext->CreateTask<LoadLocalConfigTask>(configHolder, FileManager::GetLocalConfigFilePath());
    appContext->taskManager.AddTask(std::move(loadConfigTask), loadConfigReceiver);

    Receiver updateConfigReceiver;
    updateConfigReceiver.onStarted = [](const BaseTask* task) {
        qDebug() << task->GetDescription();
    };
    updateConfigReceiver.onProgress = [](const BaseTask* task, quint32 progress) {
        std::cout << "progress: " << progress << "\r";
    };
    updateConfigReceiver.onFinished = [arguments, this](const BaseTask* task) {
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
    appContext->taskManager.AddTask(std::move(updateTask), updateConfigReceiver);
}

void InstallAndLaunchTask::OnUpdateConfigFinished(const QStringList& arguments)
{
    QString branchName = "Stable";
    if (arguments.size() >= 2)
    {
        branchName = arguments.at(1);
    }

    QString versionName = "recent";
    if (arguments.size() >= 3)
    {
        versionName = arguments.at(2);
    }

    AppVersion* localVersion = LauncherUtils::FindVersion(&configHolder->localConfig, branchName, appName, versionName);
    AppVersion* remoteVersion = LauncherUtils::FindVersion(&configHolder->remoteConfig, branchName, appName, versionName);

    Receiver runReceiver;
    runReceiver.onFinished = [this](const BaseTask* task) {
        if (task->HasError())
        {
            qDebug() << "error: " + task->GetError();
            exit(1);
        }
        else
        {
            qDebug() << "success";
            exit(0);
        }
    };

    if (remoteVersion == nullptr)
    {
        if (localVersion == nullptr)
        {
            qDebug() << "error: application" << appName << "in branch" << branchName << "with version" << versionName << "not found on local and on remote";
            exit(1);
        }
        else
        {
            Branch* branch = LauncherUtils::FindBranch(&configHolder->localConfig, branchName);
            if (branch == nullptr)
            {
                qDebug() << "branch " << branchName << "not found!";
                exit(1);
            }
            Application* app = LauncherUtils::FindApplication(branch, appName);
            if (app == nullptr)
            {
                qDebug() << "application" << appName << "int branch" << branchName << "not found!";
                exit(1);
            }
            std::unique_ptr<BaseTask> runTask = appContext->CreateTask<RunApplicationTask>(configHolder, branch->id, app->id, localVersion->id);
            appContext->taskManager.AddTask(std::move(runTask), runReceiver);
        }
    }
    else
    {
        if (localVersion != nullptr)
        {
            if (localVersion->id == remoteVersion->id)
            {
                Branch* branch = LauncherUtils::FindBranch(&configHolder->localConfig, branchName);
                if (branch == nullptr)
                {
                    qDebug() << "branch" << branchName << "not found!";
                    exit(1);
                }
                Application* app = LauncherUtils::FindApplication(branch, appName);
                if (app == nullptr)
                {
                    qDebug() << "application" << appName << "in branch" << branchName << "not found";
                    exit(1);
                }
                std::unique_ptr<BaseTask> runTask = appContext->CreateTask<RunApplicationTask>(configHolder, branch->id, app->id, localVersion->id);
                appContext->taskManager.AddTask(std::move(runTask), runReceiver);
                Q_ASSERT(false && "exit was not called after task finished");
            }
        }

        Branch* branch = LauncherUtils::FindBranch(&configHolder->remoteConfig, branchName);
        if (branch == nullptr)
        {
            qDebug() << "branch" << branchName << "not found!";
            exit(1);
        }
        Application* app = LauncherUtils::FindApplication(branch, appName);
        if (app == nullptr)
        {
            qDebug() << "application" << appName << "in branch" << branchName << "not found";
            exit(1);
        }

        InstallApplicationParams params;
        params.branch = branch->id;
        params.app = app->id;
        params.newVersion = *remoteVersion;
        params.appToStart = appName;

        Receiver installReceiver;
        installReceiver.onStarted = [](const BaseTask* task) {
            qDebug() << task->GetDescription();
        };
        installReceiver.onProgress = [](const BaseTask* task, quint32 progress) {
            std::cout << "progress: " << progress << "\r";
        };
        installReceiver.onFinished = [this](const BaseTask* task) {
            if (task->HasError())
            {
                qDebug() << "error: " + task->GetError();
                exit(1);
            }
            else if (dynamic_cast<const InstallApplicationTask*>(task) != nullptr)
            {
                qDebug() << "success";
                exit(0);
            }
        };

        std::unique_ptr<BaseTask> installTask = appContext->CreateTask<InstallApplicationTask>(configHolder, params);
        appContext->taskManager.AddTask(std::move(installTask), installReceiver);
    }
}
