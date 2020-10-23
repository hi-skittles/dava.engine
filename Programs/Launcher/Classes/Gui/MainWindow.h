#pragma once

#include "Core/Receiver.h"

#include <QMainWindow>
#include <QListWidgetItem>
#include <QtGui>
#include <QSet>
#include <QDebug>

class GuiApplicationManager;
class BranchesListModel;
class QSortFilterProxyModel;

struct Application;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum eUserType
    {
        Designer,
        Programmer,
        QA
    };

    explicit MainWindow(GuiApplicationManager* appManager, QWidget* parent = 0);
    ~MainWindow();

    void RefreshApps();

    void ShowTable(QString branchID);

    const Receiver& GetReceiver() const;

    QString GetSelectedBranchID() const;

    void ShowDebugString(const QString& str);

signals:
    void RefreshClicked();

    void ShowPreferences();
    void CancelClicked();

private slots:
    void OnListItemClicked(QModelIndex);

    void OnCellDoubleClicked(QModelIndex index);

    void OnlinkClicked(QUrl url);

    void OnRemoveAllBuilds();

private:
    void OnRemove(int index);
    void OnDownload(int index);
    void OnShowInFinder(int index);
    void OnRun(int index);
    void OnRecent(int index);
    void CopyVersion(int index);
    void OpenUrl(int index);
    void RefreshBranchesList();
    void OnConnectedChanged(bool connected);
    void AddText(const QString& text, const QColor& color = Qt::black);

    void GetTableApplicationIDs(int rowNumber, QString& appID, QString& installedVersionID, QString& avalibleVersionID);

    QWidget* CreateAppNameTableItem(const QString& stringID, const Application* localApp, int rowNum);
    QWidget* CreateAppInstalledTableItem(const QString& stringID, int rowNum);
    QWidget* CreateAppAvalibleTableItem(Application* app, Application* local, int rowNum);

    void OnTaskStarted(const BaseTask* task);
    void OnTaskProgress(const BaseTask* task, quint32 progress);
    void OnTaskFinished(const BaseTask* task);

    void OnNewsLoaded(const BaseTask* task);

    void CreateUserTypeLayout();

    Ui::MainWindow* ui = nullptr;

    QString selectedBranchID;

    QFont tableFont;
    BranchesListModel* listModel = nullptr;
    QSortFilterProxyModel* filterModel = nullptr;
    GuiApplicationManager* appManager = nullptr;

    Receiver receiver;

    QBuffer newsDataBuffer;

    int userType = Designer;
};
