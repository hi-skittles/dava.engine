#include "UI/AssetCacheServerWindow.h"
#include "ui_AssetCacheServerWidget.h"
#include "UI/SharedPoolWidget.h"
#include "UI/SharedServerWidget.h"

#include "ApplicationSettings.h"
#include "CustomServerWidget.h"

#include <AssetCache/AssetCacheConstants.h>
#include <Version/Version.h>

#include "FileSystem/KeyedArchive.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/File.h"
#include "Logger/Logger.h"
#include "Utils/StringFormat.h"
#include "Job/JobManager.h"

#include <QFileDialog>
#include <QMenu>
#include <QCloseEvent>
#include <QSettings>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QScrollBar>

#if defined(__DAVAENGINE_WINDOWS__)
#include <QSettings>
#elif defined(__DAVAENGINE_MACOS__)
#include <QXmlStreamReader>
#endif

namespace
{
const DAVA::String DEFAULT_REMOTE_IP = DAVA::AssetCache::GetLocalHost();
const DAVA::uint16 DEFAULT_REMOTE_PORT = DAVA::AssetCache::ASSET_SERVER_PORT;
}

AssetCacheServerWindow::AssetCacheServerWindow(ServerCore& core, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::AssetCacheServerWidget)
    , serverCore(core)
{
    setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowFullscreenButtonHint);

    ui->setupUi(this);

    // hint to force labels have same size (for better outlook)
    ui->numberOfFilesLabel->setFixedSize(ui->numberOfFilesLabel->sizeHint());
    // the same for spin boxes
    ui->cacheSizeSpinBox->setFixedSize(ui->cacheSizeSpinBox->sizeHint());

    DAVA::String title = DAVA::Version::CreateAppVersion("Asset Cache Server");
    setWindowTitle(QString::fromStdString(title));

    connect(ui->cacheFolderLineEdit, &QLineEdit::textChanged, this, &AssetCacheServerWindow::OnFolderTextChanged);
    connect(ui->selectFolderButton, &QPushButton::clicked, this, &AssetCacheServerWindow::OnFolderSelection);
    connect(ui->systemStartupCheckBox, SIGNAL(toggled(bool)), this, SLOT(OnSystemStartupToggled(bool)));
    connect(ui->cacheSizeSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnCacheSizeChanged(double)));
    connect(ui->clearButton, &QPushButton::clicked, this, &AssetCacheServerWindow::OnClearButtonClicked);

    connect(ui->numberOfFilesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(OnNumberOfFilesChanged(int)));
    connect(ui->autoSaveTimeoutSpinBox, SIGNAL(valueChanged(int)), this, SLOT(OnAutoSaveTimeoutChanged(int)));
    connect(ui->autoStartCheckBox, SIGNAL(toggled(bool)), this, SLOT(OnAutoStartToggled(bool)));
    connect(ui->restartCheckBox, SIGNAL(toggled(bool)), this, SLOT(OnRestartToggled(bool)));
    connect(ui->addNewServerButton, &QPushButton::clicked, this, &AssetCacheServerWindow::OnCustomServerToAdd);
    connect(ui->shareButton, &QPushButton::clicked, this, &AssetCacheServerWindow::OnShareButtonClicked);
    connect(ui->unshareButton, &QPushButton::clicked, this, &AssetCacheServerWindow::OnUnshareButtonClicked);

    connect(ui->assertButton, &QPushButton::clicked, this, &AssetCacheServerWindow::OnGenerateAssertButtonClicked);
    connect(ui->crashButton, &QPushButton::clicked, this, &AssetCacheServerWindow::OnGenerateCrashButtonClicked);

    connect(ui->applyButton, &QPushButton::clicked, this, &AssetCacheServerWindow::OnApplyButtonClicked);
    connect(ui->closeButton, &QPushButton::clicked, this, &AssetCacheServerWindow::OnCloseButtonClicked);

    connect(ui->settingsListWidget, &QListWidget::currentRowChanged, this, &AssetCacheServerWindow::ChangeSettingsPage);
    ui->settingsListWidget->setCurrentRow(0);

    serversLayout = new QVBoxLayout();
    customServersLayout = new QVBoxLayout();
    sharedPoolsLayout = new QVBoxLayout();
    serversLayout->addLayout(sharedPoolsLayout);
    serversLayout->addLayout(customServersLayout);
    ui->scrollAreaWidgetContents->setLayout(serversLayout);
    ui->scrollAreaWidgetContents->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));

    QObject::connect(ui->scrollArea->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(OnServersAreaRangeChanged(int, int)));

    CreateTrayMenu();

    ChangeSettingsState(NOT_EDITED);

    connect(&serverCore, &ServerCore::ServerStateChanged, this, &AssetCacheServerWindow::OnServerStateChanged);
    connect(&serverCore, &ServerCore::StorageSizeChanged, this, &AssetCacheServerWindow::UpdateUsageProgressbar);
    connect(&serverCore, &ServerCore::ServerShared, this, &AssetCacheServerWindow::SetVisibilityForShareControls);
    connect(&serverCore, &ServerCore::ServerUnshared, this, &AssetCacheServerWindow::SetVisibilityForShareControls);
    connect(&serverCore, &ServerCore::SharedDataUpdated, this, &AssetCacheServerWindow::UpdateSharedPoolsList);
    connect(&serverCore, &ServerCore::SharedDataUpdated, this, &AssetCacheServerWindow::UpdateSharedPoolsCombo);

    LoadSettings();
    SetupLaunchOnStartup(ui->systemStartupCheckBox->isChecked(), ui->restartCheckBox->isChecked());

    OnServerStateChanged(&serverCore);

    DAVA::uint64 occupied, overall;
    serverCore.GetStorageSpaceUsage(occupied, overall);
    UpdateUsageProgressbar(occupied, overall);
}

AssetCacheServerWindow::~AssetCacheServerWindow()
{
    if (trayIcon)
    {
        trayIcon->hide();
    }
}

void AssetCacheServerWindow::CreateTrayMenu()
{
    startAction = new QAction("Start server", this);
    connect(startAction, &QAction::triggered, this, &AssetCacheServerWindow::OnStartAction);

    stopAction = new QAction("Stop server", this);
    connect(stopAction, &QAction::triggered, this, &AssetCacheServerWindow::OnStopAction);

    QAction* editAction = new QAction("Edit settings", this);
    connect(editAction, &QAction::triggered, this, &AssetCacheServerWindow::OnEditAction);

    QAction* quitAction = new QAction("Quit", this);
    connect(quitAction, &QAction::triggered, qApp, QApplication::quit);

    QMenu* trayActionsMenu = new QMenu(this);
    trayActionsMenu->addAction(startAction);
    trayActionsMenu->addAction(stopAction);
    trayActionsMenu->addSeparator();
    trayActionsMenu->addAction(editAction);
    trayActionsMenu->addSeparator();
    trayActionsMenu->addAction(quitAction);

    QIcon windowIcon(":/icon/TrayIcon.png");
    setWindowIcon(windowIcon);

    greenGreenTrayIcon.reset(new QIcon(":/icon/TrayIcon_green_green.png"));
    greenGrayTrayIcon.reset(new QIcon(":/icon/TrayIcon_green_gray.png"));
    greenRedTrayIcon.reset(new QIcon(":/icon/TrayIcon_green_red.png"));
    redGrayTrayIcon.reset(new QIcon(":/icon/TrayIcon_red_gray.png"));

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(*redGrayTrayIcon);
    trayIcon->setToolTip("Asset Cache Server");
    trayIcon->setContextMenu(trayActionsMenu);
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated, this, &AssetCacheServerWindow::OnTrayIconActivated);
}

void AssetCacheServerWindow::ChangeSettingsPage(int newRow)
{
    ui->stackedWidget->setCurrentIndex(newRow);
}

void AssetCacheServerWindow::closeEvent(QCloseEvent* e)
{
    hide();
    e->ignore();

    if (settingsState != NOT_EDITED)
    {
        LoadSettings();
    }
}

void AssetCacheServerWindow::ChangeSettingsState(SettingsState newState)
{
    settingsState = newState;
    ui->applyButton->setEnabled(settingsState == EDITED);
}

void AssetCacheServerWindow::OnFirstLaunch()
{
    show();
}

void AssetCacheServerWindow::SetupLaunchOnStartup(bool toLaunchOnStartup, bool toRestartOnCrash)
{
#if defined(__DAVAENGINE_WINDOWS__)

    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (toLaunchOnStartup)
    {
        settings.setValue("AssetCacheServer", QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
    }
    else
    {
        settings.remove("AssetCacheServer");
    }
    
#elif defined(__DAVAENGINE_MACOS__)

    DAVA::FilePath plist("~/Library/LaunchAgents/AssetCacheServer.plist");
    DAVA::FileSystem::Instance()->DeleteFile(plist);

    if (toLaunchOnStartup)
    {
        QByteArray buffer;
        buffer.reserve(1024);
        QXmlStreamWriter xml(&buffer);

        xml.writeStartDocument();
        xml.writeDTD("<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">");
        xml.writeStartElement("plist");
        xml.writeAttribute("version", "1.0");

        xml.writeStartElement("dict");
        xml.writeTextElement("key", "Label");
        xml.writeTextElement("string", "com.davaconsulting.assetcacheserver");
        xml.writeTextElement("key", "ProgramArguments");
        xml.writeStartElement("array");
        xml.writeTextElement("string", QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
        xml.writeEndElement();
        xml.writeTextElement("key", "RunAtLoad");
        xml.writeStartElement("true");
        xml.writeEndElement();

        xml.writeTextElement("key", "KeepAlive");
        if (toRestartOnCrash)
        {
            xml.writeStartElement("dict");
            xml.writeTextElement("key", "SuccessfulExit");
            xml.writeStartElement("false");
            xml.writeEndElement();
            xml.writeEndElement();
        }
        else
        {
            xml.writeStartElement("false");
            xml.writeEndElement();
        }

        xml.writeEndElement();

        xml.writeEndElement();
        xml.writeEndDocument();

        DAVA::FileSystem::Instance()->CreateDirectory(plist.GetDirectory(), true);

        DAVA::ScopedPtr<DAVA::File> file(DAVA::File::Create(plist, DAVA::File::CREATE | DAVA::File::WRITE));
        DVASSERT(file);
        file->Write(buffer.data(), buffer.size());
    }
    
#endif
}

void AssetCacheServerWindow::OnTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::DoubleClick:
    {
        showNormal();
        activateWindow();
        raise();
        break;
    }
    default:
        break;
    }
}

void AssetCacheServerWindow::OnFolderSelection()
{
    QString startPath = ui->cacheFolderLineEdit->text();
    if (startPath.isEmpty())
    {
        startPath = QDir::currentPath();
    }

    QString directory = QFileDialog::getExistingDirectory(this, "Choose directory", startPath,
                                                          QFileDialog::ShowDirsOnly);

    if (!directory.isEmpty())
    {
        ui->cacheFolderLineEdit->setText(directory);
        VerifyChanges();
    }
}

void AssetCacheServerWindow::OnFolderTextChanged()
{
    ui->cacheFolderLineEdit->setFocus();
    VerifyChanges();
}

void AssetCacheServerWindow::OnCacheSizeChanged(double)
{
    VerifyChanges();
}

void AssetCacheServerWindow::OnNumberOfFilesChanged(int)
{
    VerifyChanges();
}

void AssetCacheServerWindow::OnAutoSaveTimeoutChanged(int)
{
    VerifyChanges();
}

void AssetCacheServerWindow::OnPortChanged(int)
{
    VerifyChanges();
}

void AssetCacheServerWindow::OnHttpPortChanged(int)
{
    VerifyChanges();
}

void AssetCacheServerWindow::OnAutoStartToggled(bool)
{
    VerifyChanges();
}

void AssetCacheServerWindow::OnSystemStartupToggled(bool checked)
{
    ui->restartCheckBox->setEnabled(checked);
    VerifyChanges();
}

void AssetCacheServerWindow::OnRestartToggled(bool)
{
    VerifyChanges();
}

void AssetCacheServerWindow::OnServersAreaRangeChanged(int min, int max)
{
    if (customServerManuallyAdded)
    {
        customServerManuallyAdded = false;
        ui->scrollArea->verticalScrollBar()->setValue(max);
    }
}

void AssetCacheServerWindow::OnCustomServerToAdd()
{
    AddCustomServer(RemoteServerParams(DEFAULT_REMOTE_IP, false));
    customServerManuallyAdded = true;
    VerifyChanges();
}

void AssetCacheServerWindow::OnCustomServerToRemove()
{
    CustomServerWidget* serverWidget = qobject_cast<CustomServerWidget*>(sender());
    customServersWidgets.remove(serverWidget);
    serverWidget->deleteLater();

    VerifyChanges();
}

void AssetCacheServerWindow::OnCustomServerEdited()
{
    VerifyChanges();
}

void AssetCacheServerWindow::OnSharedPoolChecked(bool checked)
{
    if (checked)
    {
        SharedPoolWidget* poolWidget = qobject_cast<SharedPoolWidget*>(sender());
        poolWidget->SetChecked(false);
        ClearPreviousCheck();
        poolWidget->SetChecked(true);
    }

    VerifyChanges();
}

void AssetCacheServerWindow::OnSharedServerChecked(bool checked)
{
    if (checked)
    {
        SharedServerWidget* serverWidget = qobject_cast<SharedServerWidget*>(sender());
        serverWidget->SetChecked(false);
        ClearPreviousCheck();
        serverWidget->SetChecked(true);
    }
    VerifyChanges();
}

void AssetCacheServerWindow::OnCustomServerChecked(bool checked)
{
    if (checked)
    {
        CustomServerWidget* serverWidget = qobject_cast<CustomServerWidget*>(sender());
        serverWidget->SetChecked(false);
        ClearPreviousCheck();
        serverWidget->SetChecked(true);
    }

    VerifyChanges();
}

void AssetCacheServerWindow::ClearPreviousCheck()
{
    CheckedRemote previouslyChecked = GetCheckedRemote();
    switch (previouslyChecked.type)
    {
    case CheckedRemote::POOL:
        previouslyChecked.widget.poolWidget->SetChecked(false);
        return;
    case CheckedRemote::POOL_SERVER:
        previouslyChecked.widget.serverWidget->SetChecked(false);
        return;
    case CheckedRemote::CUSTOM_SERVER:
        previouslyChecked.widget.customServerWidget->SetChecked(false);
        return;
    default:
        return;
    }
}

AssetCacheServerWindow::CheckedRemote::CheckedRemote(SharedPoolWidget* w)
    : type(AssetCacheServerWindow::CheckedRemote::POOL)
    , poolID(w->GetPoolID())
{
    widget.poolWidget = w;
}

AssetCacheServerWindow::CheckedRemote::CheckedRemote(SharedServerWidget* w)
    : type(AssetCacheServerWindow::CheckedRemote::POOL_SERVER)
    , poolID(w->GetPoolID())
    , serverID(w->GetServerID())
{
    widget.serverWidget = w;
}

AssetCacheServerWindow::CheckedRemote::CheckedRemote(CustomServerWidget* w)
    : type(AssetCacheServerWindow::CheckedRemote::CUSTOM_SERVER)
{
    widget.customServerWidget = w;
}

AssetCacheServerWindow::CheckedRemote AssetCacheServerWindow::GetCheckedRemote()
{
    for (auto& poolContainer : poolContainers)
    {
        SharedPoolContainer& pool = poolContainer.second;
        if (pool.poolWidget != nullptr && pool.poolWidget->IsChecked())
            return CheckedRemote(pool.poolWidget);

        for (auto& serverContainer : pool.serverContainers)
        {
            SharedServerContainer& server = serverContainer.second;
            if (server.serverWidget->IsChecked())
                return CheckedRemote(server.serverWidget);
        }
    }

    for (CustomServerWidget* serverWidget : customServersWidgets)
    {
        if (serverWidget->IsChecked())
            return CheckedRemote(serverWidget);
    }

    return CheckedRemote();
}

void AssetCacheServerWindow::OnEditAction()
{
    show();
    raise();
}

void AssetCacheServerWindow::OnStartAction()
{
    serverCore.Start();
}

void AssetCacheServerWindow::OnStopAction()
{
    serverCore.Stop();
}

void AssetCacheServerWindow::DisplayCurrentRemoteServer()
{
    EnabledRemote enabledRemote = serverCore.Settings().GetEnabledRemote();

    switch (enabledRemote.type)
    {
    case EnabledRemote::POOL:
    {
        ui->selectedRemoteLineEdit->setText(enabledRemote.pool->poolName.c_str());
        break;
    }
    case EnabledRemote::POOL_SERVER:
    {
        const DAVA::Map<PoolID, SharedPool>& pools = serverCore.Settings().GetSharedPools();
        const auto& found = pools.find(enabledRemote.server->poolID);
        DVASSERT(found != pools.end());
        const SharedPool& pool = found->second;
        ui->selectedRemoteLineEdit->setText(DAVA::Format("%s / %s", pool.poolName.c_str(), enabledRemote.server->serverName.c_str()).c_str());
        break;
    }
    case EnabledRemote::CUSTOM_SERVER:
    {
        ui->selectedRemoteLineEdit->setText(enabledRemote.customServer->ip.c_str());
        break;
    }
    case EnabledRemote::NONE:
    {
        ui->selectedRemoteLineEdit->setText("none");
        break;
    }
    default:
    {
        DVASSERT(false, DAVA::Format("Unexpected remote type: %u", enabledRemote.type).c_str());
    }
    }
}

void AssetCacheServerWindow::ConstructSharedPoolsList()
{
    for (auto poolContainerIter = poolContainers.begin(); poolContainerIter != poolContainers.end();)
    {
        auto deletedPoolIter = poolContainerIter++;
        RemovePoolContainer(deletedPoolIter);
    }

    const DAVA::Map<PoolID, SharedPool>& pools = serverCore.Settings().GetSharedPools();

    for (auto& poolEntry : pools)
    {
        const SharedPool& pool = poolEntry.second;
        AddPoolContainer(pool);
    }
}

void AssetCacheServerWindow::UpdateSharedPoolsCombo()
{
    int currentIndex = ui->poolComboBox->currentIndex();
    PoolID currentID = serverCore.Settings().GetOwnPoolID();
    if (currentIndex > -1)
    {
        DVASSERT(currentIndex < comboBoxIDs.size());
        currentID = comboBoxIDs[currentIndex];
    }

    ui->poolComboBox->clear();
    comboBoxIDs.clear();

    const DAVA::Map<PoolID, SharedPool>& pools = serverCore.Settings().GetSharedPools();
    comboBoxIDs.reserve(pools.size());

    auto addPoolIntoComboBox = [&](PoolID poolID, const char* poolName)
    {
        ui->poolComboBox->addItem(poolName);
        comboBoxIDs.push_back(poolID);

        if (poolID == currentID)
        {
            ui->poolComboBox->setCurrentIndex(static_cast<int>(comboBoxIDs.size()) - 1);
        }
    };

    for (auto& poolEntry : pools)
    {
        const SharedPool& pool = poolEntry.second;
        if (pool.poolID != NullPoolID)
        {
            addPoolIntoComboBox(pool.poolID, pool.poolName.c_str());
        }
    }

    addPoolIntoComboBox(NullPoolID, "none");
}

void AssetCacheServerWindow::UpdateSharedPoolsList()
{
    const DAVA::Map<PoolID, SharedPool>& pools = serverCore.Settings().GetSharedPools();

    for (auto& poolEntry : pools)
    {
        const SharedPool& pool = poolEntry.second;

        auto poolContainerFound = poolContainers.find(pool.poolID);
        if (poolContainerFound != poolContainers.end())
        {
            SharedPoolContainer& poolContainer = poolContainerFound->second;
            poolContainer.existsInUpdate = true;
            if (poolContainer.poolWidget)
                poolContainer.poolWidget->Update(pool);

            for (auto& serverEntry : pool.servers)
            {
                const SharedServer& server = serverEntry.second;
                auto serverContainterFound = poolContainer.serverContainers.find(server.serverID);
                if (serverContainterFound != poolContainer.serverContainers.end())
                {
                    SharedServerContainer& serverContainer = serverContainterFound->second;
                    serverContainer.existsInUpdate = true;
                    serverContainer.serverWidget->Update(server);
                }
                else
                {
                    SharedServerContainer& serverContainer = AddServerContainer(poolContainer, server);
                    serverContainer.existsInUpdate = true;
                }
            }
        }
        else
        {
            SharedPoolContainer& poolContainer = AddPoolContainer(pool);
            poolContainer.existsInUpdate = true;
        }
    }

    for (auto poolContainerIter = poolContainers.begin(); poolContainerIter != poolContainers.end();)
    {
        SharedPoolContainer& poolContainer = poolContainerIter->second;
        if (poolContainer.existsInUpdate)
        {
            for (auto serverContainerIter = poolContainer.serverContainers.begin(); serverContainerIter != poolContainer.serverContainers.end();)
            {
                SharedServerContainer& serverContainer = serverContainerIter->second;
                if (serverContainer.existsInUpdate)
                {
                    serverContainer.existsInUpdate = false;
                    ++serverContainerIter;
                }
                else
                {
                    auto deletedServerIter = serverContainerIter++;
                    RemoveServerContainer(poolContainer, deletedServerIter);
                }
            }

            poolContainer.existsInUpdate = false;
            ++poolContainerIter;
        }
        else
        {
            auto deletedPoolIter = poolContainerIter++;
            RemovePoolContainer(deletedPoolIter);
        }
    }

    DisplayCurrentRemoteServer();
}

AssetCacheServerWindow::SharedPoolContainer& AssetCacheServerWindow::AddPoolContainer(const SharedPool& pool)
{
    auto insertResult = poolContainers.insert(std::make_pair(pool.poolID, SharedPoolContainer()));
    DVASSERT(insertResult.second == true, DAVA::Format("Container with pool ID %u is already inserted", pool.poolID).c_str());
    SharedPoolContainer& poolContainer = insertResult.first->second;

    poolContainer.poolLayout = new QVBoxLayout();
    QVBoxLayout* poolLayout = poolContainer.poolLayout;
    sharedPoolsLayout->addLayout(poolLayout);

    if (pool.poolID == NullPoolID)
    {
        poolLayout->addWidget(new QLabel("Shared servers without pools:"));
    }
    else
    {
        poolContainer.poolWidget = new SharedPoolWidget(pool);
        connect(poolContainer.poolWidget, SIGNAL(PoolChecked(bool)), this, SLOT(OnSharedPoolChecked(bool)));
        poolLayout->addWidget(poolContainer.poolWidget);
    }

    for (const std::pair<ServerID, SharedServer>& serverEntry : pool.servers)
    {
        AddServerContainer(poolContainer, serverEntry.second);
    }

    QFrame* horizontalLine = new QFrame();
    horizontalLine->setFrameShape(QFrame::HLine);
    poolLayout->addSpacing(10);
    poolLayout->addWidget(horizontalLine);
    poolLayout->addSpacing(10);

    return poolContainer;
}

AssetCacheServerWindow::SharedServerContainer& AssetCacheServerWindow::AddServerContainer(SharedPoolContainer& poolContainer, const SharedServer& server)
{
    auto insertResult = poolContainer.serverContainers.insert(std::make_pair(server.serverID, SharedServerContainer()));
    DVASSERT(insertResult.second == true, DAVA::Format("Container with server ID %u is already inserted", server.serverID).c_str());
    SharedServerContainer& serverContainer = insertResult.first->second;

    serverContainer.serverWidget = new SharedServerWidget(server);
    connect(serverContainer.serverWidget, SIGNAL(ServerChecked(bool)), this, SLOT(OnSharedServerChecked(bool)));
    int insertAtIndex = static_cast<int>(poolContainer.serverContainers.size()) + 1;
    poolContainer.poolLayout->insertWidget(insertAtIndex, serverContainer.serverWidget);

    return serverContainer;
}

void AssetCacheServerWindow::RemoveServerContainer(SharedPoolContainer& poolContainer, ServerContainersMap::iterator iter)
{
    poolContainer.poolLayout->removeWidget(iter->second.serverWidget);
    iter->second.serverWidget->deleteLater();
    poolContainer.serverContainers.erase(iter);
}

void AssetCacheServerWindow::RemovePoolContainer(PoolContainersMap::iterator iter)
{
    QVBoxLayout* poolLayout = iter->second.poolLayout;

    ClearVBoxLayout(poolLayout);
    sharedPoolsLayout->removeItem(poolLayout);

    poolContainers.erase(iter);
}

void AssetCacheServerWindow::ConstructCustomServersList()
{
    ClearCustomServersList();

    customServersLayout->addWidget(new QLabel("User defined servers:", this));

    auto& servers = serverCore.Settings().GetCustomServers();
    for (const RemoteServerParams& sd : servers)
    {
        AddCustomServer(sd);
    }
}

void AssetCacheServerWindow::ClearCustomServersList()
{
    ClearVBoxLayout(customServersLayout);
    customServersWidgets.clear();
}

void AssetCacheServerWindow::ClearVBoxLayout(QVBoxLayout* layout)
{
    while (QLayoutItem* item = layout->takeAt(0))
    {
        layout->removeItem(item);
        if (QWidget* widget = item->widget())
        {
            widget->deleteLater();
        }
        else
        {
            delete item;
        }
    }
}

void AssetCacheServerWindow::AddCustomServer(const RemoteServerParams& newServer)
{
    CustomServerWidget* serverWidget = new CustomServerWidget(newServer, this);
    customServersWidgets.push_back(serverWidget);

    connect(serverWidget, &CustomServerWidget::RemoveLater, this, &AssetCacheServerWindow::OnCustomServerToRemove);
    connect(serverWidget, &CustomServerWidget::ParametersChanged, this, &AssetCacheServerWindow::OnCustomServerEdited);
    connect(serverWidget, SIGNAL(ServerChecked(bool)), this, SLOT(OnCustomServerChecked(bool)));

    customServersLayout->addWidget(serverWidget);

    VerifyChanges();
}

void AssetCacheServerWindow::VerifyChanges()
{
    SettingsState newState = NOT_EDITED;
    if (ui->cacheFolderLineEdit->text().isEmpty())
    {
        newState = EDITED_NOT_CORRECT;
    }
    else
    {
        newState = EDITED;
    }

    ChangeSettingsState(newState);
}

void AssetCacheServerWindow::OnClearButtonClicked()
{
    serverCore.ClearStorage();
}

void AssetCacheServerWindow::OnShareButtonClicked()
{
    serverCore.InitiateShareRequest(comboBoxIDs[ui->poolComboBox->currentIndex()], ui->serverNameEdit->text().toStdString());
}

void AssetCacheServerWindow::OnUnshareButtonClicked()
{
    serverCore.InitiateUnshareRequest();
}

void AssetCacheServerWindow::OnGenerateAssertButtonClicked()
{
    DAVA::Logger::Info("Manual assert generation");
    DVASSERT(false, "Manual assert generation");
}

void AssetCacheServerWindow::OnGenerateCrashButtonClicked()
{
    DAVA::Logger::Info("Manual crash generation");
    int* a = nullptr;
    *a = 1;
}

void AssetCacheServerWindow::OnApplyButtonClicked()
{
    bool toLaunchOnStartup = ui->systemStartupCheckBox->isChecked();
    bool toRestartOnCrash = ui->restartCheckBox->isChecked();
    if (serverCore.Settings().IsLaunchOnSystemStartup() != toLaunchOnStartup)
    {
        SetupLaunchOnStartup(toLaunchOnStartup, toRestartOnCrash);
    }

    SaveSettings();
}

void AssetCacheServerWindow::OnCloseButtonClicked()
{
    hide();

    if (settingsState != NOT_EDITED)
    {
        LoadSettings();
    }
}

void AssetCacheServerWindow::SaveSettings()
{
    serverCore.Settings().SetFolder(ui->cacheFolderLineEdit->text().toStdString());
    serverCore.Settings().SetCacheSizeGb(ui->cacheSizeSpinBox->value());
    serverCore.Settings().SetFilesCount(ui->numberOfFilesSpinBox->value());
    serverCore.Settings().SetAutoSaveTimeoutMin(ui->autoSaveTimeoutSpinBox->value());
    serverCore.Settings().SetAutoStart(ui->autoStartCheckBox->isChecked());
    serverCore.Settings().SetLaunchOnSystemStartup(ui->systemStartupCheckBox->isChecked());
    serverCore.Settings().SetRestartOnCrash(ui->restartCheckBox->isChecked());

    serverCore.Settings().DisableRemote();

    serverCore.Settings().ClearCustomServers();
    for (auto& server : customServersWidgets)
    {
        serverCore.Settings().AddCustomServer(server->GetServerData());
    }

    CheckedRemote checkedRemote = GetCheckedRemote();
    if (checkedRemote.type == CheckedRemote::POOL)
        serverCore.Settings().EnableSharedPool(checkedRemote.poolID);
    else if (checkedRemote.type == CheckedRemote::POOL_SERVER)
        serverCore.Settings().EnableSharedServer(checkedRemote.poolID, checkedRemote.serverID);

    serverCore.Settings().Save();

    DisplayCurrentRemoteServer();
    ChangeSettingsState(NOT_EDITED);
}

void AssetCacheServerWindow::LoadSettings()
{
    bool blocked = blockSignals(true);

    ui->cacheFolderLineEdit->setText(serverCore.Settings().GetFolder().GetAbsolutePathname().c_str());
    ui->cacheSizeSpinBox->setValue(serverCore.Settings().GetCacheSizeGb());
    ui->numberOfFilesSpinBox->setValue(serverCore.Settings().GetFilesCount());
    ui->autoSaveTimeoutSpinBox->setValue(serverCore.Settings().GetAutoSaveTimeoutMin());
    ui->autoStartCheckBox->setChecked(serverCore.Settings().IsAutoStart());
    ui->systemStartupCheckBox->setChecked(serverCore.Settings().IsLaunchOnSystemStartup());
    ui->restartCheckBox->setEnabled(serverCore.Settings().IsLaunchOnSystemStartup());
    ui->restartCheckBox->setChecked(serverCore.Settings().IsRestartOnCrash());

    ConstructSharedPoolsList();
    ConstructCustomServersList();

    DisplayCurrentRemoteServer();

    UpdateSharedPoolsCombo();
    ui->serverNameEdit->setText(serverCore.Settings().GetOwnName().c_str());
    SetVisibilityForShareControls();

    blockSignals(blocked);

    ChangeSettingsState(NOT_EDITED);
}

void AssetCacheServerWindow::OnServerStateChanged(const ServerCore* server)
{
    DVASSERT(&serverCore == server);

    auto serverState = serverCore.GetState();
    auto remoteState = serverCore.GetRemoteState();

    switch (serverState)
    {
    case ServerCore::State::STARTED:
    {
        switch (remoteState)
        {
        case ServerCore::RemoteState::STARTED:
        {
            trayIcon->setIcon(*greenGreenTrayIcon);
            break;
        }
        case ServerCore::RemoteState::CONNECTING:
        case ServerCore::RemoteState::VERIFYING:
        case ServerCore::RemoteState::WAITING_REATTEMPT:
        {
            trayIcon->setIcon(*greenRedTrayIcon);
            break;
        }
        case ServerCore::RemoteState::STOPPED:
        {
            trayIcon->setIcon(*greenGrayTrayIcon);
            break;
        }
        default:
        {
            DVASSERT(false && "Unknown remote state");
        }
        }

        startAction->setDisabled(true);
        stopAction->setEnabled(true);
        break;
    }
    case ServerCore::State::STOPPED:
    {
        trayIcon->setIcon(*redGrayTrayIcon);
        startAction->setEnabled(true);
        stopAction->setDisabled(true);
        break;
    }
    }
}

void AssetCacheServerWindow::UpdateUsageProgressbar(DAVA::uint64 occupied, DAVA::uint64 overall)
{
    DAVA::float64 p = overall ? (100. / static_cast<DAVA::float64>(overall)) : 0;
    int occupiedInPercents = static_cast<int>(p * static_cast<DAVA::float64>(occupied));
    if (occupiedInPercents == 0 && occupied > 0)
    {
        // If storage size vanishes, then 0 percent of usage is shown, yet some data is actually present.
        // There we are forcing to display 1 percent to indicate that storage is not empty.
        occupiedInPercents = 1;
    }
    ui->occupiedSizeBar->setRange(0, 100);
    ui->occupiedSizeBar->setValue(occupiedInPercents);
}

void AssetCacheServerWindow::SetVisibilityForShareControls()
{
    bool isSharedNow = serverCore.Settings().IsSharedForOthers();
    ui->poolComboBox->setEnabled(!isSharedNow);
    ui->serverNameEdit->setEnabled(!isSharedNow);
    ui->shareButton->setEnabled(!isSharedNow);
    ui->unshareButton->setEnabled(isSharedNow);
}
