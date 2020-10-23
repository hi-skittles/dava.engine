#pragma once

#include "Data/ConfigParser.h"
#include "Core/ApplicationContext.h"
#include "Core/Receiver.h"
#include "Core/ApplicationQuitController.h"
#include "Core/CommonTasks/InstallApplicationTask.h"
#include "Core/TaskManager.h"
#include "Core/TasksLogger.h"
#include "Core/ConfigHolder.h"
#include "Gui/MainWindow.h"
#include "Utils/Utils.h"

#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>

#include <memory>

struct AppVersion;
class AppsCommandsSender;
class BAManagerClient;
class ConfigRefresher;
class FileDownloader;
class UrlsHolder;

class GuiApplicationManager : public QObject
{
    Q_OBJECT

public:
    explicit GuiApplicationManager(ApplicationQuitController* quitController);
    ~GuiApplicationManager();

    void Start();

    void AddTaskWithBaseReceivers(std::unique_ptr<BaseTask>&& task);
    void AddTaskWithCustomReceivers(std::unique_ptr<BaseTask>&& task, std::vector<Receiver> receivers);

    template <typename T, typename... Arguments>
    void AddTaskWithBaseReceivers(Arguments&&... args);

    void InstallApplication(const InstallApplicationParams& params);

    QString GetString(const QString& stringID) const;

    ConfigHolder* GetConfigHolder();

    MainWindow* GetMainWindow() const;

    void ShowApplicataionInExplorer(const QString& branchID, const QString& appID);
    void RunApplication(const QString& branchID, const QString& appID);
    void RunApplication(const QString& branchID, const QString& appID, const QString& versionID);

    void RemoveApplication(const QString& branchID, const QString& appID);

    void CheckUpdates();

    ApplicationContext* GetContext() const;

private slots:
    void Refresh();
    void OpenPreferencesEditor();

private:
    Notifier notifier;

    TasksLogger tasksLogger;

    ApplicationQuitController* quitController = nullptr;

    AppsCommandsSender* commandsSender = nullptr;

    BAManagerClient* baManagerClient = nullptr;
    ConfigRefresher* configRefresher = nullptr;

    MainWindow* mainWindow = nullptr;

    ConfigHolder configHolder;

    ApplicationContext* context = nullptr;
};

template <typename T, typename... Arguments>
void GuiApplicationManager::AddTaskWithBaseReceivers(Arguments&&... args)
{
    AddTaskWithBaseReceivers(std::move(context->CreateTask<T>(std::forward<Arguments>(args)...)));
}
