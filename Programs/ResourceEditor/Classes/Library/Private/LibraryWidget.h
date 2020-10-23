#pragma once

#include "TArc/Core/FieldBinder.h"

#include <QWidget>
#include <QTreeView>
#include <QItemSelection>
#include <QStringList>

#include <memory>

namespace DAVA
{
class ContextAccessor;
}

class QVBoxLayout;
class QToolBar;
class QAction;
class QLineEdit;
class QComboBox;
class QProgressBar;
class QLabel;
class QSpacerItem;

class LibraryFileSystemModel;
class LibraryFilteringModel;
class REFileOperationsManager;

//TreeView with custom signals
class LibraryTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit LibraryTreeView(QWidget* parent = 0)
        : QTreeView(parent){};

signals:
    void DragStarted();

protected:
    virtual void startDrag(Qt::DropActions supportedActions)
    {
        emit DragStarted();
        QTreeView::startDrag(supportedActions);
    }
};

class LibraryWidget : public QWidget
{
    Q_OBJECT

    enum eViewMode
    {
        VIEW_AS_LIST = 0,
        VIEW_DETAILED
    };

public:
    LibraryWidget(DAVA::ContextAccessor* contextAccessor, std::weak_ptr<REFileOperationsManager> fileOpMng, QWidget* parent = 0);

signals:

    void AddSceneRequested(const DAVA::FilePath& scenePathname);
    void EditSceneRequested(const DAVA::FilePath& scenePathname);

    void DAEConvertionRequested(const DAVA::FilePath& daePathname);
    void DAEAnimationConvertionRequested(const DAVA::FilePath& daePathname);

    void FBXConvertionRequested(const DAVA::FilePath& fbxPathname);
    void FBXAnimationConvertionRequested(const DAVA::FilePath& fbxPathname);

    void DoubleClicked(const DAVA::FilePath& scenePathname);
    void DragStarted();

protected slots:
    void ViewAsList();
    void ViewDetailed();

    void SelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void ShowContextMenu(const QPoint& point);
    void fileDoubleClicked(const QModelIndex& index);

    void OnFilesTypeChanged(int typeIndex);

    void OnAddModel();
    void OnEditModel();

    void OnConvertDae();
    void OnConvertAnimationsDae();

    void OnConvertFBX();
    void OnConvertAnimationsFBX();

    void OnRevealAtFolder();

private:
    void OnProjectChanged(const DAVA::Any& projectFieldValue);

    void SetupFileTypes();
    void SetupToolbar();
    void SetupView();
    void SetupLayout();

    void HideDetailedColumnsAtFilesView(bool show);

private:
    QVBoxLayout* layout = nullptr;

    QToolBar* toolbar = nullptr;
    QTreeView* filesView = nullptr;

    QComboBox* filesTypeFilter = nullptr;

    QAction* actionViewAsList = nullptr;
    QAction* actionViewDetailed = nullptr;

    QString rootPathname;
    LibraryFileSystemModel* filesModel = nullptr;

    eViewMode viewMode;
    int curTypeIndex = -1;

    std::unique_ptr<DAVA::FieldBinder> fieldBinder;
    DAVA::ContextAccessor* contextAccessor = nullptr;
    std::weak_ptr<REFileOperationsManager> fileOperationsManager;
};
