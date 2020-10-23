#include "Core/GuiApplicationManager.h"
#include "Core/BAManagerClient.h"
#include "Core/ConfigRefresher.h"
#include "Core/UrlsHolder.h"

#include "Core/CommonTasks/RunApplicationTask.h"
#include "Core/CommonTasks/RemoveApplicationTask.h"
#include "Core/CommonTasks/LoadLocalConfigTask.h"
#include "Core/CommonTasks/UpdateConfigTask.h"
#include "Core/CommonTasks/SelfUpdateTask.h"

#include "Gui/PreferencesDialog.h"

#include "Utils/AppsCommandsSender.h"
#include "Utils/FileManager.h"
#include "Utils/ErrorMessenger.h"

#include <QtHelpers/HelperFunctions.h>
#include <QtHelpers/ProcessHelper.h>

#include <QFile>
#include <QDebug>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QShortcut>

#include <atomic>
#include <thread>
#include <chrono>

GuiApplicationManager::GuiApplicationManager(ApplicationQuitController* quitController_)
    : quitController(quitController_)
    , commandsSender(new AppsCommandsSender(this))
    , baManagerClient(new BAManagerClient(this))
    , configRefresher(new ConfigRefresher(this))
    , mainWindow(new MainWindow(this))
    , context(new ApplicationContext())
{
    using namespace std::placeholders;

    connect(mainWindow, &MainWindow::ShowPreferences, this, &GuiApplicationManager::OpenPreferencesEditor);

    connect(mainWindow, &MainWindow::CancelClicked, &context->taskManager, &TaskManager::Terminate);

    //create secret shortcut
    //it will be used to get commands manually for testing reasons
    QShortcut* shortCut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_G), mainWindow);
    shortCut->setContext(Qt::ApplicationShortcut);
    connect(shortCut, &QShortcut::activated, baManagerClient, &BAManagerClient::AskForCommands);

    connect(mainWindow, &MainWindow::RefreshClicked, this, &GuiApplicationManager::Refresh);
    connect(configRefresher, &ConfigRefresher::RefreshConfig, this, &GuiApplicationManager::Refresh);

    Receiver receiver;
    receiver.onFinished = [this](const BaseTask* task) {
        mainWindow->RefreshApps();
    };

    PreferencesDialog::LoadPreferences(context, baManagerClient, configRefresher);
    std::unique_ptr<BaseTask> task = context->CreateTask<LoadLocalConfigTask>(&configHolder, FileManager::GetLocalConfigFilePath());
    AddTaskWithCustomReceivers(std::move(task), { receiver, mainWindow->GetReceiver() });

    //Debug key to dump memory
    connect(new QShortcut(Qt::Key_H, mainWindow), &QShortcut::activated, [this]() {
        QString debugStr = QString("total objects size: %1, mainWindow size: %2, tasks count: %3")
                           .arg(findChildren<QObject*>().size())
                           .arg(mainWindow->findChildren<QObject*>().size())
                           .arg(context->taskManager.GetTasksCount());
        ErrorMessenger::LogMessage(QtDebugMsg, debugStr);
        mainWindow->ShowDebugString(debugStr);
    });
}

GuiApplicationManager::~GuiApplicationManager()
{
    PreferencesDialog::SavePreferences(context, baManagerClient, configRefresher);

    context->taskManager.Terminate();
    delete mainWindow;
    delete context;
}

void GuiApplicationManager::Start()
{
    //this code is used to cleanup temp directories after self-update
    context->fileManager.DeleteDirectory(context->fileManager.GetTempDirectory());

    mainWindow->show();

    Refresh();
}

void GuiApplicationManager::AddTaskWithBaseReceivers(std::unique_ptr<BaseTask>&& task)
{
    AddTaskWithCustomReceivers(std::move(task), { mainWindow->GetReceiver() });
}

void GuiApplicationManager::AddTaskWithCustomReceivers(std::unique_ptr<BaseTask>&& task, std::vector<Receiver> receivers)
{
    receivers.push_back(tasksLogger.GetReceiver());
    Notifier notifier(receivers);
    context->taskManager.AddTask(std::move(task), notifier);
}

void GuiApplicationManager::InstallApplication(const InstallApplicationParams& params)
{
    Receiver receiver;

    receiver.onFinished = [this, params](const BaseTask* task) {
        if (task->GetTaskType() != BaseTask::ASYNC_CHAIN)
        {
            return;
        }

        mainWindow->RefreshApps();
        if (task->HasError() == false)
        {
            CheckUpdates();
        }
        if (mainWindow->GetSelectedBranchID() == params.branch)
        {
            mainWindow->ShowTable(mainWindow->GetSelectedBranchID());
        }
    };
    std::unique_ptr<BaseTask> task = context->CreateTask<InstallApplicationTask>(&configHolder, params);
    AddTaskWithCustomReceivers(std::move(task), { receiver, mainWindow->GetReceiver() });
}

void GuiApplicationManager::Refresh()
{
    Receiver receiver;

    receiver.onFinished = [this](const BaseTask* task) {
        if (task->GetTaskType() != BaseTask::ASYNC_CHAIN)
        {
            return;
        }
        mainWindow->RefreshApps();
        if (task->HasError() == false)
        {
            configHolder.localConfig.SaveToFile(FileManager::GetLocalConfigFilePath());
            CheckUpdates();
        }
    };

    std::unique_ptr<BaseTask> task = context->CreateTask<UpdateConfigTask>(&configHolder, context->urlsHolder.GetURLs());
    AddTaskWithCustomReceivers(std::move(task), { receiver, mainWindow->GetReceiver() });
}

void GuiApplicationManager::OpenPreferencesEditor()
{
    PreferencesDialog::ShowPreferencesDialog(context, configRefresher, mainWindow);
}

void GuiApplicationManager::CheckUpdates()
{
#ifndef __DAVAENGINE_DEBUG__
    //check self-update
    if (configHolder.remoteConfig.GetLauncherVersion() != configHolder.localConfig.GetLauncherVersion())
    {
        AddTaskWithBaseReceivers<SelfUpdateTask>(quitController, configHolder.remoteConfig.GetLauncherURL());
        return;
    }
#endif //__DAVAENGINE_DEBUG__

    if (context->isBuildAgent)
    {
        //check applications update
        int branchCount = configHolder.remoteConfig.GetBranchCount();
        for (int i = 0; i < branchCount; ++i)
        {
            bool toolsetWasFound = false;
            Branch* remoteBranch = configHolder.remoteConfig.GetBranch(i);
            if (!configHolder.localConfig.GetBranch(remoteBranch->id))
                continue;

            int appCount = remoteBranch->GetAppCount();
            for (int j = 0; j < appCount; ++j)
            {
                Application* remoteApp = remoteBranch->GetApplication(j);
                if (!configHolder.localConfig.GetApplication(remoteBranch->id, remoteApp->id))
                    continue;

                if (remoteApp->GetVerionsCount() == 1)
                {
                    AppVersion* remoteAppVersion = remoteApp->GetVersion(0);
                    Application* localApp = configHolder.localConfig.GetApplication(remoteBranch->id, remoteApp->id);
                    AppVersion* localAppVersion = localApp->GetVersion(0);
                    if (localAppVersion->id != remoteAppVersion->id)
                    {
                        if (remoteAppVersion->isToolSet)
                        {
                            if (toolsetWasFound)
                            {
                                continue;
                            }
                            toolsetWasFound = true;
                        }
                        InstallApplicationParams params;
                        params.branch = remoteBranch->id;
                        params.app = remoteApp->id;
                        params.newVersion = *remoteAppVersion;
                        InstallApplication(params);
                    }
                }
            }
        }
    }
}

ApplicationContext* GuiApplicationManager::GetContext() const
{
    return context;
}

QString GuiApplicationManager::GetString(const QString& stringID) const
{
    QString string = configHolder.remoteConfig.GetString(stringID);
    if (string == stringID)
        string = configHolder.localConfig.GetString(stringID);
    return string;
}

ConfigHolder* GuiApplicationManager::GetConfigHolder()
{
    return &configHolder;
}

MainWindow* GuiApplicationManager::GetMainWindow() const
{
    return mainWindow;
}

void GuiApplicationManager::ShowApplicataionInExplorer(const QString& branchID, const QString& appID)
{
    AppVersion* installedVersion = configHolder.localConfig.GetAppVersion(branchID, appID);
    if (installedVersion == nullptr)
    {
        return;
    }
    QString appDirPath = LauncherUtils::GetApplicationDirectory(&configHolder, context, branchID, appID, installedVersion->isToolSet);
    if (QFile::exists(appDirPath))
    {
        QtHelpers::ShowInOSFileManager(appDirPath);
    }
}

void GuiApplicationManager::RunApplication(const QString& branchID, const QString& appID)
{
    AppVersion* installedAppVersion = configHolder.localConfig.GetAppVersion(branchID, appID);
    if (installedAppVersion == nullptr)
    {
        return;
    }
    RunApplication(branchID, appID, installedAppVersion->id);
}

void GuiApplicationManager::RunApplication(const QString& branchID, const QString& appID, const QString& versionID)
{
    AddTaskWithBaseReceivers<RunApplicationTask>(&configHolder, branchID, appID, versionID);
}

void GuiApplicationManager::RemoveApplication(const QString& branchID, const QString& appID)
{
    AddTaskWithBaseReceivers<RemoveApplicationTask>(&configHolder, branchID, appID);
}
