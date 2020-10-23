#pragma once

#include "EditorSystems/EditorSystemsManager.h"

#include "UI/Preview/ScaleComboBoxAdapter.h"
#include "UI/Preview/ScrollBarAdapter.h"

#include <TArc/DataProcessing/DataWrapper.h>
#include <TArc/Utils/QtConnections.h>

#include <Engine/Qt/IClientDelegate.h>
#include <Engine/Qt/RenderWidget.h>

#include <QFrame>
#include <QPointer>

namespace DAVA
{
class ContextAccessor;
class OperationInvoker;
class UI;
class DataContext;
}
class EditorSystemsManager;
class WidgetsData;

class ControlNode;
class PackageBaseNode;
class AbstractProperty;

class FindInDocumentWidget;
class RulerWidget;
class RulerController;
class GuidesController;

class QGridLayout;
class QComboBox;
class QScrollBar;
class QWheelEvent;
class QNativeGestureEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
class QMenu;

namespace Interfaces
{
class PackageActionsInterface;
}

class PreviewWidget : public QFrame, private DAVA::IClientDelegate
{
    Q_OBJECT
public:
    explicit PreviewWidget(DAVA::ContextAccessor* accessor, DAVA::OperationInvoker* invoker, DAVA::UI* ui, DAVA::RenderWidget* renderWidget, EditorSystemsManager* systemsManager);
    ~PreviewWidget();

    DAVA::Signal<DAVA::uint64> requestCloseTab;
    DAVA::Signal<ControlNode*> requestChangeTextInNode;
    DAVA::Signal<bool> droppingFile;

    void RegisterPackageActions(Interfaces::PackageActionsInterface* packageActions);
    void UnregisterPackageActions();

signals:
    void OpenPackageFiles(const QStringList& links);

public slots:
    void OnIncrementScale();
    void OnDecrementScale();
    void SetActualScale();

private slots:
    void OnTabBarContextMenuRequested(const QPoint& pos);

private:
    struct LayerNode
    {
        ControlNode* control = nullptr;
        bool enabled = false;
        DAVA::String leadingSpaces;
        DAVA::List<LayerNode> subLayers;
    };

    void InitUI();
    void ShowMenu(const QMouseEvent* mouseEvent);
    void AddPickLayerMenuSection(QMenu* parentMenu, const QPoint& pos);
    void AddChangeTextMenuSection(QMenu* parentMenu, const QPoint& pos);
    void AddPackageMenuSections(QMenu* parentMenu);
    void AddBgrColorMenuSection(QMenu* parentMenu);

    bool CanChangeTextInControl(const ControlNode* node) const;

    DAVA::List<ControlNode*> GetPathFromRoot(ControlNode* node);
    void InsertControlIntoLayer(LayerNode& layer, DAVA::List<ControlNode*>& pathFromRoot);
    void InsertLayerIntoMenu(LayerNode& layer, QMenu* menu);

    void InjectRenderWidget(DAVA::RenderWidget* renderWidget);

    void CreateActions();
    void OnMouseReleased(QMouseEvent* event) override;
    void OnMouseMove(QMouseEvent* event) override;
    void OnMouseDBClick(QMouseEvent* event) override;
    void OnDragEntered(QDragEnterEvent* event) override;
    void OnDragMoved(QDragMoveEvent* event) override;
    bool ProcessDragMoveEvent(QDropEvent* event);
    void OnDragLeaved(QDragLeaveEvent* event) override;
    void OnDrop(QDropEvent* event) override;
    void OnKeyPressed(QKeyEvent* event) override;

    DAVA::ContextAccessor* accessor = nullptr;
    DAVA::OperationInvoker* invoker = nullptr;
    DAVA::UI* ui = nullptr;

    DAVA::RenderWidget* renderWidget = nullptr;

    RulerController* rulerController = nullptr;

    QAction* selectAllAction = nullptr;
    QAction* focusNextChildAction = nullptr;
    QAction* focusPreviousChildAction = nullptr;

    EditorSystemsManager* systemsManager = nullptr;
    Interfaces::PackageActionsInterface* packageActions = nullptr;

    //we can show model dialogs only when mouse released, so remember node to change text when mouse will be released
    ControlNode* nodeToChangeTextOnMouseRelease = nullptr;

    ScaleComboBoxAdapter scaleComboBoxData;
    ScrollBarAdapter hScrollBarData;
    ScrollBarAdapter vScrollBarData;
    //adapter to change scale by actions increment/decrement/normalize scale
    CanvasDataAdapter canvasDataAdapter;

    DAVA::QtConnections connections;
};
