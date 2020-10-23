#include "ui_mainwindow.h"
#include "Gui/MainWindow.h"
#include "Gui/Models/BranchesListModel.h"
#include "Gui/Models/BranchesFilterModel.h"

#include "Data/ConfigParser.h"

#include "Core/GuiApplicationManager.h"
#include "Core/CommonTasks/DownloadTask.h"

#include "Utils/ErrorMessenger.h"

#include "defines.h"

#include <QSet>
#include <QQueue>
#include <QInputDialog>
#include <QMenu>
#include <QLabel>
#include <QVariant>
#include <QComboBox>
#include <QMessageBox>

namespace MainWindowDetails
{
const QString stateKey = "mainWindow_state";
const QString geometryKey = "mainWindow_geometry";
const QString selectedBranchKey = "mainWindow_selectedBranch";
const QString userTypeKey = "mainWindow_userType";

struct ApplicationInfo
{
    Application* remote = nullptr;
    Application* local = nullptr;
};

const char* installedPropertyName = "installed";

QMap<int, QString> userTypes = {
    { MainWindow::Designer, QObject::tr("Designer") },
    { MainWindow::Programmer, QObject::tr("Programmer") },
    { MainWindow::QA, QObject::tr("QA") }
};
}

class BranchListComparator
{
public:
    bool operator()(const QString& left, const QString& right) const
    {
        int leftValue, rightValue;
        if (sscanf(left.toStdString().c_str(), "%d", &leftValue) > 0 &&
            sscanf(right.toStdString().c_str(), "%d", &rightValue) > 0 &&
            leftValue != rightValue)
        {
            return leftValue > rightValue;
        }

        return left < right;
    }
};

MainWindow::MainWindow(GuiApplicationManager* appManager_, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , appManager(appManager_)
{
    ui->setupUi(this);

    QSettings settings;
    restoreGeometry(settings.value(MainWindowDetails::geometryKey).toByteArray());
    restoreState(settings.value(MainWindowDetails::stateKey).toByteArray());
    selectedBranchID = settings.value(MainWindowDetails::selectedBranchKey).toString();
    userType = settings.value(MainWindowDetails::userTypeKey).toInt();

    CreateUserTypeLayout();

    ui->textEdit_launcherStatus->document()->setMaximumBlockCount(1000);
    ui->textEdit_launcherStatus->setReadOnly(true);

    ui->action_updateConfiguration->setShortcuts(QList<QKeySequence>() << QKeySequence("F5") << QKeySequence("Ctrl+R"));
    connect(ui->action_updateConfiguration, &QAction::triggered, this, &MainWindow::RefreshClicked);

    connect(ui->actionPreferences, &QAction::triggered, this, &MainWindow::ShowPreferences);
    connect(ui->pushButton_cancel, &QPushButton::clicked, this, &MainWindow::CancelClicked);

    connect(ui->actionRemove_all_local_builds, &QAction::triggered, this, &MainWindow::OnRemoveAllBuilds);

    ui->tableWidget->setStyleSheet(TABLE_STYLESHEET);
    ui->tableWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    ui->listView->setStyleSheet("QListView::item::selected {background: #805CADFF }");

    setWindowTitle(QString("DAVA Launcher %1").arg(LAUNCHER_VER));

    connect(ui->textBrowser, &QTextBrowser::anchorClicked, this, &MainWindow::OnlinkClicked);
    connect(ui->listView, &QListView::clicked, this, &MainWindow::OnListItemClicked);
    connect(ui->tableWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(OnCellDoubleClicked(QModelIndex)));

    listModel = new BranchesListModel(appManager, this);
    filterModel = new BranchesFilterModel(this);
    filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    connect(ui->lineEdit_search, &QLineEdit::textChanged, filterModel, &QSortFilterProxyModel::setFilterFixedString);
    filterModel->setSourceModel(listModel);
    ui->listView->setModel(filterModel);

    using namespace std::placeholders;
    receiver.onStarted = std::bind(&MainWindow::OnTaskStarted, this, _1);
    receiver.onProgress = std::bind(&MainWindow::OnTaskProgress, this, _1, _2);
    receiver.onFinished = std::bind(&MainWindow::OnTaskFinished, this, _1);

    OnConnectedChanged(false);
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue(MainWindowDetails::geometryKey, saveGeometry());
    settings.setValue(MainWindowDetails::stateKey, saveState());
    settings.setValue(MainWindowDetails::selectedBranchKey, selectedBranchID);
    settings.setValue(MainWindowDetails::userTypeKey, userType);
    SafeDelete(ui);
}

void MainWindow::OnlinkClicked(QUrl url)
{
    QDesktopServices::openUrl(url);
}

void MainWindow::OnRemoveAllBuilds()
{
    QMessageBox::warning(this, tr("removing tools"), tr("Close all opened applications and press OK"));
    AddText(tr("Removing folder DAVATools"), Qt::darkBlue);
    bool buildsRemoved = FileManager::DeleteDirectory(appManager->GetContext()->fileManager.GetBaseAppsDirectory());
    if (buildsRemoved)
    {
        appManager->GetConfigHolder()->localConfig.branches.clear();
        appManager->GetConfigHolder()->localConfig.SaveToFile(FileManager::GetLocalConfigFilePath());
        RefreshApps();
    }
    else
    {
        QMessageBox::critical(this, tr("removing tools"), tr("Failed to remove folder DAVATools :( Now you need to reset your PC and restart operation"));
    }
}

void MainWindow::OnCellDoubleClicked(QModelIndex index)
{
    if (index.column() != COLUMN_BUTTONS)
        OnRun(index.row());
}

const Receiver& MainWindow::GetReceiver() const
{
    return receiver;
}

QString MainWindow::GetSelectedBranchID() const
{
    return selectedBranchID;
}

void MainWindow::ShowDebugString(const QString& str)
{
    AddText(str, Qt::blue);
}

void MainWindow::OnShowInFinder(int rowNum)
{
    QString appID, insVersionID, avVersionID;
    GetTableApplicationIDs(rowNum, appID, insVersionID, avVersionID);
    appManager->ShowApplicataionInExplorer(selectedBranchID, appID);
}

void MainWindow::OnRun(int rowNumber)
{
    QString appID, insVersionID, avVersionID;
    GetTableApplicationIDs(rowNumber, appID, insVersionID, avVersionID);

    if (insVersionID.isEmpty() == false && (avVersionID.isEmpty() || insVersionID == avVersionID))
    {
        appManager->RunApplication(selectedBranchID, appID, insVersionID);
    }
    else
    {
        ConfigHolder* configHolder = appManager->GetConfigHolder();
        AppVersion* newVersion = configHolder->remoteConfig.GetAppVersion(selectedBranchID, appID, avVersionID);
        if (newVersion == nullptr)
        {
            ErrorMessenger::LogMessage(QtCriticalMsg, "can not found remote application or version OnInstall");
            return;
        }
        InstallApplicationParams params;
        params.branch = selectedBranchID;
        params.app = appID;
        params.newVersion = *newVersion;
        params.appToStart = appID;
        appManager->InstallApplication(params);
    }
}

void MainWindow::OnRecent(int rowNumber)
{
    QWidget* cell = ui->tableWidget->cellWidget(rowNumber, COLUMN_APP_VERSION);
    QComboBox* cBox = dynamic_cast<QComboBox*>(cell);
    if (cBox != nullptr && userType != Designer)
    {
        cBox->setCurrentIndex(0);
    }
}

void MainWindow::CopyVersion(int rowNumber)
{
    QWidget* cell = ui->tableWidget->cellWidget(rowNumber, COLUMN_APP_VERSION);
    QString text;
    QComboBox* cbx = qobject_cast<QComboBox*>(cell);
    if (cbx != nullptr)
    {
        text = cbx->currentText();
    }
    else
    {
        text = cell->property(DAVA_CUSTOM_PROPERTY_NAME).toString();
    }
    QApplication::clipboard()->setText(text);
    AddText(tr("Version %1 copied to the clipboard").arg(text), Qt::darkGreen);
}

void MainWindow::OpenUrl(int index)
{
    QString appID, insVersionID, avVersionID;
    GetTableApplicationIDs(index, appID, insVersionID, avVersionID);
    QString url = appManager->GetConfigHolder()->remoteConfig.GetAppVersion(selectedBranchID, appID, avVersionID)->url;
    QApplication::clipboard()->setText(url);
    AddText(tr("Url copied to the clipboard"), Qt::darkGreen);
}

void MainWindow::OnDownload(int rowNumber)
{
    QString appID, insVersionID, avVersionID;
    GetTableApplicationIDs(rowNumber, appID, insVersionID, avVersionID);

    ConfigHolder* configHolder = appManager->GetConfigHolder();
    AppVersion* newVersion = configHolder->remoteConfig.GetAppVersion(selectedBranchID, appID, avVersionID);
    if (newVersion == nullptr)
    {
        ErrorMessenger::LogMessage(QtCriticalMsg, "can not found remote application or version OnInstall");
        return;
    }
    InstallApplicationParams params;
    params.branch = selectedBranchID;
    params.app = appID;
    params.newVersion = *newVersion;
    appManager->InstallApplication(params);
}

void MainWindow::OnRemove(int rowNumber)
{
    QString appID, insVersionID, avVersionID;
    GetTableApplicationIDs(rowNumber, appID, insVersionID, avVersionID);
    if (!appID.isEmpty() && !insVersionID.isEmpty())
    {
        appManager->RemoveApplication(selectedBranchID, appID);
        RefreshApps();
    }
}

void MainWindow::RefreshApps()
{
    RefreshBranchesList();

    QAbstractItemModel* model = ui->listView->model();
    QModelIndex first = model->index(0, 0);
    QModelIndexList foundIndexes = model->match(first, BranchesListModel::DAVA_WIDGET_ROLE, selectedBranchID, 1, Qt::MatchExactly);
    if (foundIndexes.isEmpty() == false)
    {
        ui->listView->setCurrentIndex(foundIndexes.first());
    }
    ShowTable(selectedBranchID);
}

void MainWindow::OnListItemClicked(QModelIndex qindex)
{
    QVariant var = ui->listView->model()->data(qindex, BranchesListModel::DAVA_WIDGET_ROLE);
    QString dataRole = var.toString();
    if (!dataRole.isEmpty())
    {
        selectedBranchID = dataRole;
        ShowTable(selectedBranchID);
    }
}

void MainWindow::OnNewsLoaded(const BaseTask* task)
{
    QString error = task->GetError();
    if (error.isEmpty())
    {
        ui->textBrowser->setHtml(newsDataBuffer.data());
    }
    else
    {
        ui->textBrowser->setText(QObject::tr("Failed to load news: %1").arg(error));
    }
    newsDataBuffer.close();
}

void MainWindow::CreateUserTypeLayout()
{
    QButtonGroup* buttonGroup = new QButtonGroup(this);
    QHBoxLayout* userTypeLayout = new QHBoxLayout();
    userTypeLayout->setContentsMargins(0, 0, 0, 0);
    userTypeLayout->setSpacing(0);
    QWidget* container = new QWidget();
    container->setLayout(userTypeLayout);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // toolBar is a pointer to an existing toolbar
    ui->toolBar_actions->addWidget(spacer);

    ui->toolBar_actions->addWidget(container);
    auto createUserTypeButton = [buttonGroup, userTypeLayout](int id, const QString& buttonText, bool checked) {
        QPushButton* button = new QPushButton(buttonText);
        button->setCheckable(true);
        button->setChecked(checked);
        buttonGroup->addButton(button, id);
        userTypeLayout->addWidget(button);
    };

    for (auto iter = MainWindowDetails::userTypes.begin(); iter != MainWindowDetails::userTypes.end(); ++iter)
    {
        createUserTypeButton(iter.key(), iter.value(), iter.key() == userType);
    }

    connect(buttonGroup, static_cast<void (QButtonGroup::*)(int, bool)>(&QButtonGroup::buttonToggled), [this](int id, bool toggled) {
        if (toggled)
        {
            userType = id;
            RefreshApps();
        }
    });
}

void MainWindow::ShowTable(QString branchID)
{
    ConfigHolder* configHolder = appManager->GetConfigHolder();

    if (branchID.isEmpty() || branchID == CONFIG_LAUNCHER_WEBPAGE_KEY)
    {
        if (configHolder->localConfig.GetWebpageURL().isEmpty() == false)
        {
            ui->stackedWidget->setCurrentIndex(0);
            QString description = QObject::tr("Loading news");
            ui->textBrowser->setText(description);
            Receiver receiver;
            newsDataBuffer.open(QIODevice::ReadWrite);
            receiver.onFinished = std::bind(&MainWindow::OnNewsLoaded, this, std::placeholders::_1);
            std::unique_ptr<BaseTask> task = appManager->GetContext()->CreateTask<DownloadTask>(description, configHolder->localConfig.GetWebpageURL(), &newsDataBuffer);
            appManager->AddTaskWithCustomReceivers(std::move(task), { receiver });
            return;
        }
        else
        {
            Branch* firstAvailableBranch = configHolder->localConfig.GetBranch(0);
            if (firstAvailableBranch != nullptr)
            {
                branchID = firstAvailableBranch->id;
            }
        }
    }

    ui->stackedWidget->setCurrentIndex(1);
    ui->tableWidget->setRowCount(0);

    QMap<QString, MainWindowDetails::ApplicationInfo> applications;
    Branch* remoteBranch = configHolder->remoteConfig.GetBranch(branchID);
    if (remoteBranch != nullptr)
    {
        for (Application& app : remoteBranch->applications)
        {
            MainWindowDetails::ApplicationInfo& info = applications[app.id];
            info.remote = &app;
        }
    }

    Branch* localBranch = configHolder->localConfig.GetBranch(branchID);
    if (localBranch != nullptr)
    {
        for (Application& app : localBranch->applications)
        {
            MainWindowDetails::ApplicationInfo& info = applications[app.id];
            info.local = &app;
        }
    }

    ui->tableWidget->setRowCount(applications.size());
    int index = 0;
    for (QMap<QString, MainWindowDetails::ApplicationInfo>::Iterator iter = applications.begin(); iter != applications.end(); ++iter, ++index)
    {
        ui->tableWidget->setCellWidget(index, COLUMN_APP_NAME, CreateAppNameTableItem(iter.key(), iter->local, index));
        if (iter->remote != nullptr)
        {
            ui->tableWidget->setCellWidget(index, COLUMN_APP_VERSION, CreateAppAvalibleTableItem(iter->remote, iter->local, index));
        }

        QWidget* buttonsWidget = new QWidget(this);
        buttonsWidget->setLayout(new QHBoxLayout());
        buttonsWidget->layout()->setContentsMargins(0, 0, 0, 0);
        buttonsWidget->layout()->setSpacing(0);
        QSize buttonSize(32, 32);
        QSize iconSize(25, 25);

        auto createButton = [this, buttonSize, iconSize, index, buttonsWidget](const QString& toolTip, const QString& iconName, std::function<void()> f) {
            QPushButton* button = new QPushButton(this);
            button->setIcon(QIcon(":/Icons/" + iconName + ".png"));
            button->setFixedSize(buttonSize);
            button->setIconSize(iconSize);
            button->setToolTip(toolTip);
            connect(button, &QPushButton::clicked, f);
            buttonsWidget->layout()->addWidget(button);
            return button;
        };

        bool canRefreshRecent = false;
        if (iter->remote != nullptr && iter->remote->GetVerionsCount() > 1 && iter->local != nullptr && iter->local->versions.empty() == false)
        {
            canRefreshRecent = iter->remote->versions.back().id != iter->local->versions[0].id;
        }

        if (userType != Designer)
        {
            createButton(tr("Most recent version available!"), "recent", std::bind(&MainWindow::OnRecent, this, index))->setEnabled(canRefreshRecent);
        }

        if (userType != Designer)
        {
            createButton(tr("Download application only"), "download", std::bind(&MainWindow::OnDownload, this, index))->setEnabled(iter->remote != nullptr);
        }
        createButton(tr("Download and run application"), "run", std::bind(&MainWindow::OnRun, this, index))->setEnabled(iter->local != nullptr || iter->remote != nullptr);
        if (userType != Designer)
        {
            createButton(tr("Remove application"), "delete", std::bind(&MainWindow::OnRemove, this, index))->setEnabled(iter->local != nullptr);
            createButton(tr("Show in file system"), "show_in_finder", std::bind(&MainWindow::OnShowInFinder, this, index))->setEnabled(iter->local != nullptr);
        }
        if (userType == QA)
        {
            createButton(tr("Copy version"), "copy", std::bind(&MainWindow::CopyVersion, this, index));
            createButton(tr("Copy url"), "url", std::bind(&MainWindow::OpenUrl, this, index))->setEnabled(iter->remote != nullptr);
        }
        ui->tableWidget->setCellWidget(index, COLUMN_BUTTONS, buttonsWidget);
    }

    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QHeaderView* hHeader = ui->tableWidget->horizontalHeader();
    hHeader->setSectionResizeMode(COLUMN_APP_NAME, QHeaderView::ResizeToContents);
    hHeader->setSectionResizeMode(COLUMN_APP_VERSION, QHeaderView::Stretch);
    hHeader->setSectionResizeMode(COLUMN_BUTTONS, QHeaderView::ResizeToContents);

    ui->tableWidget->setMaximumHeight(ui->tableWidget->sizeHint().height());
}

void MainWindow::OnConnectedChanged(bool connected)
{
    QString text = QString("BA-Manager is ") + (connected ? "connected" : "disconnected");
    ui->label_BAManagerStatusText->setText(text);
    QString iconPath = QString(":/Icons/") + (connected ? "green" : "red") + ".png";
    ui->label_BAManagerStatusIcon->setPixmap(QPixmap(iconPath));
}

void MainWindow::OnTaskProgress(const BaseTask* /*task*/, quint32 progress)
{
    ui->progressBar->setValue(progress);
}

void MainWindow::AddText(const QString& text, const QColor& color)
{
    QTextEdit* textEdit = ui->textEdit_launcherStatus;
    QTextCursor textCursor = textEdit->textCursor();
    textCursor.movePosition(QTextCursor::End);
    textEdit->setTextCursor(textCursor);
    QString html = QTime::currentTime().toString() + " : " + "<font color=\"" + color.name() + "\">" + text + "</font>";
    //insertHtml method doesn't recalculate blocks and doesn't remove first blocks if number of blocks > maximum blocks count
    textEdit->append(html);
}

void MainWindow::OnTaskStarted(const BaseTask* task)
{
    AddText(task->GetDescription());

    BaseTask::eTaskType type = task->GetTaskType();
    if (type == BaseTask::DOWNLOAD_TASK || type == BaseTask::ZIP_TASK)
    {
        ui->stackedWidget_status->setCurrentIndex(0);
        ui->progressBar->setFormat(task->GetDescription() + " %p%");
    }
}

void MainWindow::OnTaskFinished(const BaseTask* task)
{
    const QString& error = task->GetError();
    if (error.isEmpty() == false)
    {
        AddText(error, "#aa0000");
    }
    BaseTask::eTaskType type = task->GetTaskType();
    if (type == BaseTask::DOWNLOAD_TASK)
    {
        const DownloadTask* downloadTask = static_cast<const DownloadTask*>(task);
        if (downloadTask->IsCancelled() == false)
        {
            OnConnectedChanged(error.isEmpty());
        }
    }
    if (type == BaseTask::DOWNLOAD_TASK || type == BaseTask::ZIP_TASK)
    {
        ui->stackedWidget_status->setCurrentIndex(1);
    }
}

void MainWindow::RefreshBranchesList()
{
    ConfigHolder* configHolder = appManager->GetConfigHolder();
    listModel->ClearItems();

    if (!configHolder->localConfig.GetWebpageURL().isEmpty())
    {
        listModel->AddItem(CONFIG_LAUNCHER_WEBPAGE_KEY, BranchesListModel::LIST_ITEM_NEWS);
        listModel->AddItem("", BranchesListModel::LIST_ITEM_SEPARATOR);
    }

    QStringList favs;
    QSet<QString> branchIDs;
    configHolder->localConfig.MergeBranchesIDs(branchIDs);
    favs = configHolder->localConfig.GetFavorites();
    configHolder->remoteConfig.MergeBranchesIDs(branchIDs);

    QList<QString> branchesList = branchIDs.toList();
    qSort(branchesList.begin(), branchesList.end(), BranchListComparator());

    int branchesCount = branchesList.size();

    //Add favorites branches
    bool hasFavorite = false;
    for (int i = 0; i < favs.size(); ++i)
    {
        const QString& branchID = favs[i];
        if (branchesList.contains(branchID))
        {
            listModel->AddItem(branchID, BranchesListModel::LIST_ITEM_FAVORITES);
            hasFavorite = true;
        }
    }
    if (hasFavorite)
        listModel->AddItem("", BranchesListModel::LIST_ITEM_SEPARATOR);

    //Add Others
    for (int i = 0; i < branchesCount; i++)
    {
        const QString& branchID = branchesList[i];
        if (!favs.contains(branchID))
            listModel->AddItem(branchID, BranchesListModel::LIST_ITEM_BRANCH);
    }
}

QWidget* MainWindow::CreateAppNameTableItem(const QString& stringID, const Application* localApp, int rowNum)
{
    QString string = appManager->GetString(stringID);
    QLabel* item = new QLabel(string);

    item->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(item, &QLabel::customContextMenuRequested, [this, item, rowNum](const QPoint& pos) {
        QString appID, insVersionID, avVersionID;
        GetTableApplicationIDs(rowNum, appID, insVersionID, avVersionID);

        AppVersion* version = appManager->GetConfigHolder()->remoteConfig.GetAppVersion(selectedBranchID, appID, avVersionID);
        if (version == nullptr)
        {
            return;
        }
        QMenu menu(this);
        QAction* copyURLAction = menu.addAction("Copy " + appID + " URL");
        QAction* selectedAction = menu.exec(ui->tableWidget->viewport()->mapToGlobal(pos) + item->pos());
        if (selectedAction == copyURLAction)
        {
            QApplication::clipboard()->setText(version->url);
        }
    });

    item->setProperty(DAVA_CUSTOM_PROPERTY_NAME, stringID);
    QString localAppName = (localApp == nullptr || localApp->versions.isEmpty()) ? "" : localApp->versions[0].id;
    item->setProperty(MainWindowDetails::installedPropertyName, localAppName);
    item->setFont(tableFont);

    return item;
}

QWidget* MainWindow::CreateAppInstalledTableItem(const QString& stringID, int rowNum)
{
    QLabel* item = new QLabel(appManager->GetString(stringID));
    item->setFont(tableFont);
    item->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    item->setContextMenuPolicy(Qt::CustomContextMenu);
    item->setProperty(DAVA_CUSTOM_PROPERTY_NAME, stringID);
    connect(item, &QLabel::customContextMenuRequested, [this, item, rowNum](const QPoint& pos) {
        QMenu menu(this);
        QAction* copyAction = menu.addAction("Copy version");
        QString actionText =
#if defined(Q_OS_WIN)
        "Show in explorer";
#elif defined(Q_OS_MAC)
        "Show in finder";
#endif //platform
        QAction* showInFinderAction = menu.addAction(actionText);
        QAction* resultAction = menu.exec(ui->tableWidget->viewport()->mapToGlobal(pos) + item->pos());

        if (resultAction == copyAction)
        {
            CopyVersion(rowNum);
        }
        else if (resultAction == showInFinderAction)
        {
            OnShowInFinder(rowNum);
        }
    });

    return item;
}

QWidget* MainWindow::CreateAppAvalibleTableItem(Application* remote, Application* local, int rowNum)
{
    int versCount = remote->GetVerionsCount();
    if (versCount == 1)
    {
        return CreateAppInstalledTableItem(remote->GetVersion(0)->id, rowNum);
    }
    else
    {
        QComboBox* comboBox = new QComboBox();
        connect(comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [comboBox, this, rowNum](int inex) {
            if (userType != Designer)
            {
                QWidget* buttons = ui->tableWidget->cellWidget(rowNum, COLUMN_BUTTONS);
                if (buttons != nullptr)
                {
                    QWidget* refreshButton = buttons->layout()->itemAt(0)->widget();
                    if (refreshButton != nullptr)
                    {
                        refreshButton->setEnabled(comboBox->currentIndex() != 0);
                    }
                }
            }
        });
        for (int j = versCount - 1; j >= 0; --j)
        {
            comboBox->addItem(remote->versions[j].id);
        }
        comboBox->view()->setTextElideMode(Qt::ElideLeft);
        comboBox->setFont(tableFont);
        comboBox->setFocusPolicy(Qt::NoFocus);

        comboBox->setCurrentIndex(0);
        if (local != nullptr && local->versions.empty() == false)
        {
            for (int i = 0; i < versCount; ++i)
            {
                if (remote->versions[i].id == local->versions[0].id)
                {
                    comboBox->setCurrentIndex(versCount - i - 1);
                }
            }
        }

        comboBox->setProperty(DAVA_CUSTOM_PROPERTY_NAME, comboBox->currentText());

#ifdef Q_OS_MAC
        comboBox->setMaximumHeight(26);
#endif

        return comboBox;
    }
}

void MainWindow::GetTableApplicationIDs(int rowNumber, QString& appID,
                                        QString& installedVersionID, QString& avalibleVersionID)
{
    QWidget* cell = 0;

    cell = ui->tableWidget->cellWidget(rowNumber, COLUMN_APP_NAME);
    if (cell)
    {
        appID = cell->property(DAVA_CUSTOM_PROPERTY_NAME).toString();
        installedVersionID = cell->property(MainWindowDetails::installedPropertyName).toString();
    }

    cell = ui->tableWidget->cellWidget(rowNumber, COLUMN_APP_VERSION);
    QComboBox* cBox = dynamic_cast<QComboBox*>(cell);
    if (cBox)
        avalibleVersionID = cBox->currentText();
    else if (cell)
        avalibleVersionID = cell->property(DAVA_CUSTOM_PROPERTY_NAME).toString();
}
