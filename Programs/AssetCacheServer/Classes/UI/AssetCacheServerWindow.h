#pragma once

#include <memory>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QLabel>

#include "ServerCore.h"

class QMenu;
class QVBoxLayout;

class SharedPoolWidget;
class SharedServerWidget;
class CustomServerWidget;
class ApplicationSettings;
struct RemoteServerParams;
struct SharedPool;

namespace Ui
{
class AssetCacheServerWidget;
}

class AssetCacheServerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AssetCacheServerWindow(ServerCore& core, QWidget* parent = nullptr);
    ~AssetCacheServerWindow() override;
    void OnFirstLaunch();

private slots:
    void OnTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void ChangeSettingsPage(int newRow);

    void OnEditAction();
    void OnStartAction();
    void OnStopAction();

    void OnFolderSelection();
    void OnFolderTextChanged();
    void OnCacheSizeChanged(double);
    void OnNumberOfFilesChanged(int);
    void OnAutoSaveTimeoutChanged(int);
    void OnPortChanged(int);
    void OnHttpPortChanged(int);
    void OnAutoStartToggled(bool);
    void OnSystemStartupToggled(bool);
    void OnRestartToggled(bool);

    void OnServersAreaRangeChanged(int, int);
    void OnCustomServerToAdd();
    void OnCustomServerToRemove();
    void OnCustomServerEdited();
    void OnCustomServerChecked(bool);
    void OnSharedPoolChecked(bool);
    void OnSharedServerChecked(bool);

    void OnShareButtonClicked();
    void OnUnshareButtonClicked();
    void OnGenerateAssertButtonClicked();
    void OnGenerateCrashButtonClicked();
    void OnClearButtonClicked();
    void OnApplyButtonClicked();
    void OnCloseButtonClicked();

    void OnServerStateChanged(const ServerCore*);
    void UpdateUsageProgressbar(DAVA::uint64, DAVA::uint64);

private:
    struct CheckedRemote
    {
        enum Type
        {
            POOL,
            POOL_SERVER,
            CUSTOM_SERVER,
            NONE
        } type = NONE;
        PoolID poolID = NullPoolID;
        ServerID serverID = NullServerID;

        union CheckedWidget
        {
            SharedPoolWidget* poolWidget = nullptr;
            SharedServerWidget* serverWidget;
            CustomServerWidget* customServerWidget;
        } widget;

        CheckedRemote() = default;
        CheckedRemote(SharedPoolWidget* w);
        CheckedRemote(SharedServerWidget* w);
        CheckedRemote(CustomServerWidget* w);
    };

    struct SharedServerContainer
    {
        bool existsInUpdate = false;
        QVBoxLayout* poolLayout = nullptr;
        SharedServerWidget* serverWidget = nullptr;
    };

    using ServerContainersMap = DAVA::Map<ServerID, SharedServerContainer>;

    struct SharedPoolContainer
    {
        bool existsInUpdate = false;
        SharedPoolWidget* poolWidget = nullptr;
        ServerContainersMap serverContainers;
        QVBoxLayout* poolLayout = nullptr;
    };

    using PoolContainersMap = DAVA::Map<PoolID, SharedPoolContainer>;

private:
    void CreateTrayMenu();

    void closeEvent(QCloseEvent* e) override; // QWidget

    void LoadSettings();
    void SaveSettings();

    void ConstructCustomServersList();
    void ClearCustomServersList();

    void ConstructSharedPoolsList();
    void UpdateSharedPoolsList();

    void ConstructSharedPoolsCombo();
    void UpdateSharedPoolsCombo();

    void ClearVBoxLayout(QVBoxLayout* layout);

    void SetVisibilityForShareControls();

    SharedPoolContainer& AddPoolContainer(const SharedPool& pool);
    SharedServerContainer& AddServerContainer(SharedPoolContainer& poolContainer, const SharedServer& server);
    void RemovePoolContainer(PoolContainersMap::iterator iter);
    void RemoveServerContainer(SharedPoolContainer& poolContainer, ServerContainersMap::iterator iter);

    void AddCustomServer(const RemoteServerParams& newServer);

    void ClearPreviousCheck();
    CheckedRemote GetCheckedRemote();

    void DisplayCurrentRemoteServer();

    void SetupLaunchOnStartup(bool toLaunchOnStartup, bool toRestartOnCrash);

    void VerifyChanges();

    enum SettingsState
    {
        NOT_EDITED,
        EDITED,
        EDITED_NOT_CORRECT
    };

    void ChangeSettingsState(SettingsState newState);

private:
    std::unique_ptr<Ui::AssetCacheServerWidget> ui;
    QAction* startAction = nullptr;
    QAction* stopAction = nullptr;
    QSystemTrayIcon* trayIcon = nullptr;
    std::unique_ptr<QIcon> greenGreenTrayIcon;
    std::unique_ptr<QIcon> greenGrayTrayIcon;
    std::unique_ptr<QIcon> greenRedTrayIcon;
    std::unique_ptr<QIcon> redGrayTrayIcon;

    QVBoxLayout* customServersLayout = nullptr;
    QVBoxLayout* sharedPoolsLayout = nullptr;
    QVBoxLayout* serversLayout = nullptr;

    bool customServerManuallyAdded = false;

    DAVA::List<CustomServerWidget*> customServersWidgets;
    PoolContainersMap poolContainers;
    DAVA::Vector<PoolID> comboBoxIDs;

    ServerCore& serverCore;

    SettingsState settingsState = NOT_EDITED;
    bool showAdvanced = false;
};
