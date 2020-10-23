#include "Classes/Application/QEGlobal.h"
#include "Classes/Controls/ScaleComboBox.h"
#include "Classes/EditorSystems/EditorSystemsManager.h"
#include "Classes/Interfaces/PackageActionsInterface.h"
#include "Classes/Model/ControlProperties/RootProperty.h"
#include "Classes/Model/ControlProperties/VisibleValueProperty.h"
#include "Classes/Model/PackageHierarchy/ImportedPackagesNode.h"
#include "Classes/Model/PackageHierarchy/StyleSheetsNode.h"
#include "Classes/Model/PackageHierarchy/PackageBaseNode.h"
#include "Classes/Model/PackageHierarchy/PackageControlsNode.h"
#include "Classes/Model/PackageHierarchy/PackageNode.h"
#include "Classes/Model/PackageHierarchy/StyleSheetsNode.h"
#include "Classes/Modules/CanvasModule/CanvasData.h"
#include "Classes/Modules/CanvasModule/CanvasData.h"
#include "Classes/Modules/DocumentsModule/DocumentData.h"
#include "Classes/Modules/DocumentsModule/EditorSystemsData.h"
#include "Classes/Modules/HUDModule/HUDModuleData.h"
#include "Classes/Modules/PackageModule/PackageMimeData.h"
#include "Classes/Modules/ProjectModule/ProjectData.h"
#include "Classes/UI/CommandExecutor.h"
#include "Classes/UI/Preview/Data/CentralWidgetData.h"
#include "Classes/UI/Preview/Guides/GuidesController.h"
#include "Classes/UI/Preview/PreviewWidget.h"
#include "Classes/UI/Preview/PreviewWidgetSettings.h"
#include "Classes/UI/Preview/Ruler/RulerController.h"
#include "Classes/UI/Preview/Ruler/RulerWidget.h"
#include "Classes/Utils/DragNDropHelper.h"

#include <TArc/Controls/SceneTabbar.h>
#include <TArc/Controls/ScrollBar.h>
#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/Core/ContextAccessor.h>
#include <TArc/Core/OperationInvoker.h>
#include <TArc/DataProcessing/DataContext.h>
#include <TArc/DataProcessing/Common.h>
#include <TArc/WindowSubSystem/QtAction.h>

#include <QtTools/Updaters/ContinuousUpdater.h>

#include <UI/UIControl.h>
#include <UI/Text/UITextComponent.h>
#include <UI/UIControlSystem.h>
#include <Engine/Engine.h>
#include <Reflection/ReflectedTypeDB.h>

#include <QLineEdit>
#include <QScreen>
#include <QMenu>
#include <QShortcut>
#include <QFileInfo>
#include <QInputDialog>
#include <QComboBox>
#include <QScrollBar>
#include <QGridLayout>
#include <QApplication>
#include <QTimer>
#include <QLabel>

PreviewWidget::PreviewWidget(DAVA::ContextAccessor* accessor_, DAVA::OperationInvoker* invoker_, DAVA::UI* ui_, DAVA::RenderWidget* renderWidget, EditorSystemsManager* systemsManager_)
    : QFrame(nullptr)
    , accessor(accessor_)
    , invoker(invoker_)
    , ui(ui_)
    , rulerController(new RulerController(accessor, this))
    , scaleComboBoxData(accessor)
    , hScrollBarData(DAVA::Vector2::AXIS_X, accessor)
    , vScrollBarData(DAVA::Vector2::AXIS_Y, accessor)
    , canvasDataAdapter(accessor)
    , systemsManager(systemsManager_)
{
    InjectRenderWidget(renderWidget);
    InitUI();
}

PreviewWidget::~PreviewWidget() = default;

void PreviewWidget::CreateActions()
{
    using namespace DAVA;

    selectAllAction = new QAction(tr("Select all"), this);
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    selectAllAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    renderWidget->addAction(selectAllAction);
    connect(selectAllAction, &QAction::triggered, std::bind(&EditorSystemsManager::SelectAll, systemsManager));

    FieldDescriptor fieldDescr;
    fieldDescr.type = ReflectedTypeDB::Get<EditorSystemsData>();
    fieldDescr.fieldName = FastName(EditorSystemsData::emulationModePropertyName);

    QtAction* focusNextChildAction = new QtAction(accessor, tr("Focus next child"), this);
    focusNextChildAction->setShortcut(Qt::Key_Tab);
    focusNextChildAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    renderWidget->addAction(focusNextChildAction);

    focusNextChildAction->SetStateUpdationFunction(QtAction::Enabled, fieldDescr, [](const Any& fieldValue) -> Any {
        return fieldValue.Cast<bool>(false) == false;
    });
    connect(focusNextChildAction, &QAction::triggered, std::bind(&EditorSystemsManager::FocusNextChild, systemsManager));

    QtAction* focusPreviousChildAction = new QtAction(accessor, tr("Focus previous child"), this);
    focusPreviousChildAction->setShortcut(static_cast<int>(Qt::ShiftModifier | Qt::Key_Tab));
    focusPreviousChildAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    renderWidget->addAction(focusPreviousChildAction);
    focusPreviousChildAction->SetStateUpdationFunction(QtAction::Enabled, fieldDescr, [](const Any& fieldValue) -> Any {
        return fieldValue.Cast<bool>(false) == false;
    });
    connect(focusPreviousChildAction, &QAction::triggered, std::bind(&EditorSystemsManager::FocusPreviousChild, systemsManager));
}

void PreviewWidget::OnIncrementScale()
{
    using namespace DAVA;

    DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    CanvasData* canvasData = activeContext->GetData<CanvasData>();

    float32 nextScale = canvasData->GetNextScale(1);
    canvasDataAdapter.SetScale(nextScale);
}

void PreviewWidget::OnDecrementScale()
{
    using namespace DAVA;

    DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    CanvasData* canvasData = activeContext->GetData<CanvasData>();

    float32 nextScale = canvasData->GetPreviousScale(-1);
    canvasDataAdapter.SetScale(nextScale);
}

void PreviewWidget::SetActualScale()
{
    using namespace DAVA;

    canvasDataAdapter.SetScale(1.0f);
}

void PreviewWidget::InjectRenderWidget(DAVA::RenderWidget* renderWidget_)
{
    DVASSERT(renderWidget_ != nullptr);
    renderWidget = renderWidget_;
    CreateActions();

    renderWidget->SetClientDelegate(this);
}

void PreviewWidget::InitUI()
{
    using namespace DAVA;

    GuidesController* hGuidesController = new GuidesController(Vector2::AXIS_X, accessor, this);
    GuidesController* vGuidesController = new GuidesController(Vector2::AXIS_Y, accessor, this);

    QVBoxLayout* vLayout = new QVBoxLayout(this);

    DAVA::DataContext* ctx = accessor->GetGlobalContext();
    DAVA::SceneTabbar* tabBar = new DAVA::SceneTabbar(accessor, DAVA::Reflection::Create(&accessor), this);
    addActions(tabBar->actions());
    tabBar->closeTab.Connect(&requestCloseTab, &DAVA::Signal<DAVA::uint64>::Emit);
    tabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tabBar, &QWidget::customContextMenuRequested, this, &PreviewWidget::OnTabBarContextMenuRequested);

    tabBar->setElideMode(Qt::ElideNone);
    tabBar->setTabsClosable(true);
    tabBar->setUsesScrollButtons(true);
    vLayout->addWidget(tabBar);

    QGridLayout* gridLayout = new QGridLayout();
    vLayout->addLayout(gridLayout);
    RulerWidget* horizontalRuler = new RulerWidget(accessor, hGuidesController, this);
    horizontalRuler->SetRulerOrientation(Qt::Horizontal);
    horizontalRuler->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    connect(rulerController, &RulerController::HorisontalRulerSettingsChanged, horizontalRuler, &RulerWidget::OnRulerSettingsChanged);
    connect(rulerController, &RulerController::HorisontalRulerMarkPositionChanged, horizontalRuler, &RulerWidget::OnMarkerPositionChanged);
    gridLayout->addWidget(horizontalRuler, 0, 1);

    RulerWidget* verticalRuler = new RulerWidget(accessor, vGuidesController, this);
    verticalRuler->SetRulerOrientation(Qt::Vertical);
    verticalRuler->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
    connect(rulerController, &RulerController::VerticalRulerSettingsChanged, verticalRuler, &RulerWidget::OnRulerSettingsChanged);
    connect(rulerController, &RulerController::VerticalRulerMarkPositionChanged, verticalRuler, &RulerWidget::OnMarkerPositionChanged);
    gridLayout->addWidget(verticalRuler, 1, 0);

    DAVA::DataContext* globalContext = accessor->GetGlobalContext();
    globalContext->CreateData(std::make_unique<CentralWidgetData>(renderWidget, horizontalRuler, verticalRuler));

    QSizePolicy expandingPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    renderWidget->setSizePolicy(expandingPolicy);
    gridLayout->addWidget(renderWidget, 1, 1);
    {
        ScrollBar::Params params(accessor, ui, DAVA::mainWindowKey);
        params.fields[ScrollBar::Fields::Value] = ScrollBarAdapter::positionPropertyName;
        params.fields[ScrollBar::Fields::Minimum] = ScrollBarAdapter::minPosPropertyName;
        params.fields[ScrollBar::Fields::Maximum] = ScrollBarAdapter::maxPosPropertyName;
        params.fields[ScrollBar::Fields::PageStep] = ScrollBarAdapter::pageStepPropertyName;
        params.fields[ScrollBar::Fields::Orientation] = ScrollBarAdapter::orientationPropertyName;
        params.fields[ScrollBar::Fields::Enabled] = ScrollBarAdapter::enabledPropertyName;
        params.fields[ScrollBar::Fields::Visible] = ScrollBarAdapter::visiblePropertyName;
        {
            ScrollBar* scrollBar = new ScrollBar(params, accessor, Reflection::Create(ReflectedObject(&vScrollBarData)));
            scrollBar->ForceUpdate();
            gridLayout->addWidget(scrollBar->ToWidgetCast(), 1, 2);
        }

        {
            ScrollBar* scrollBar = new ScrollBar(params, accessor, Reflection::Create(ReflectedObject(&hScrollBarData)));
            scrollBar->ForceUpdate();
            gridLayout->addWidget(scrollBar->ToWidgetCast(), 2, 1);
        }
    }

    gridLayout->setMargin(0.0f);
    gridLayout->setSpacing(1.0f);

    vLayout->setMargin(0.0f);
    vLayout->setSpacing(1.0f);

    {
        ScaleComboBox::Params params(accessor, ui, DAVA::mainWindowKey);
        params.fields[ScaleComboBox::Fields::Enumerator] = ScaleComboBoxAdapter::enumeratorPropertyName;
        params.fields[ScaleComboBox::Fields::Value] = ScaleComboBoxAdapter::scalePropertyName;
        params.fields[ScaleComboBox::Fields::Enabled] = ScaleComboBoxAdapter::enabledPropertyName;
        ScaleComboBox* scaleCombo = new ScaleComboBox(params, accessor, Reflection::Create(ReflectedObject(&scaleComboBoxData)));

        QString toolbarName = "Document toolBar";
        ActionPlacementInfo toolBarScalePlacement(CreateMenuPoint(QList<QString>() << "View"
                                                                                   << "Toolbars"));
        ui->DeclareToolbar(DAVA::mainWindowKey, toolBarScalePlacement, toolbarName);

        QAction* action = new QAction(nullptr);
        QWidget* container = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(container);
        layout->setMargin(0);
        layout->addWidget(new QLabel(tr("Scale")));
        layout->addWidget(scaleCombo->ToWidgetCast());
        layout->addWidget(new QLabel(tr("%")));

        AttachWidgetToAction(action, container);

        ActionPlacementInfo placementInfo(CreateToolbarPoint(toolbarName));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }
}

void PreviewWidget::ShowMenu(const QMouseEvent* mouseEvent)
{
    QMenu menu;

    QPoint localPos = mouseEvent->pos();
    AddPickLayerMenuSection(&menu, localPos);
    AddChangeTextMenuSection(&menu, localPos);
    AddPackageMenuSections(&menu);
    AddBgrColorMenuSection(&menu);

    if (menu.actions().isEmpty() == false)
    {
        menu.exec(mouseEvent->globalPos());
    }
}

void PreviewWidget::AddPickLayerMenuSection(QMenu* menu, const QPoint& pos)
{
    using namespace DAVA;

    Vector<ControlNode*> nodesUnderPoint;
    Vector2 davaPos(pos.x(), pos.y());
    auto predicateForMenu = [davaPos](const ControlNode* node) -> bool
    {
        const UIControl* control = node->GetControl();
        DVASSERT(nullptr != control);
        const VisibleValueProperty* visibleProp = node->GetRootProperty()->GetVisibleProperty();
        return visibleProp->GetVisibleInEditor() && control->IsPointInside(davaPos);
    };
    auto stopPredicate = [](const ControlNode* node) -> bool {
        const auto visibleProp = node->GetRootProperty()->GetVisibleProperty();
        return !visibleProp->GetVisibleInEditor();
    };
    systemsManager->CollectControlNodes(std::back_inserter(nodesUnderPoint), predicateForMenu, stopPredicate);

    LayerNode rootLayer;
    for (ControlNode* control : nodesUnderPoint)
    {
        List<ControlNode*> path = GetPathFromRoot(control);
        InsertControlIntoLayer(rootLayer, path);
    }

    QMenu* pickLayerMenu = new QMenu("Pick Layer");
    pickLayerMenu->setEnabled(nodesUnderPoint.empty() == false);
    menu->addMenu(pickLayerMenu);
    menu->addSeparator();
    InsertLayerIntoMenu(rootLayer, pickLayerMenu);
}

void PreviewWidget::AddChangeTextMenuSection(QMenu* menu, const QPoint& localPos)
{
    DAVA::Vector2 davaPoint(localPos.x(), localPos.y());
    ControlNode* node = systemsManager->GetControlNodeAtPoint(davaPoint);
    if (CanChangeTextInControl(node))
    {
        QString name = QString::fromStdString(node->GetName());
        QAction* action = menu->addAction(tr("Change text in %1").arg(name));
        connect(action, &QAction::triggered, [this, node]() { requestChangeTextInNode.Emit(node); });
    }
}

void PreviewWidget::AddPackageMenuSections(QMenu* parentMenu)
{
    if (packageActions != nullptr)
    {
        parentMenu->addAction(packageActions->GetCutAction());
        parentMenu->addAction(packageActions->GetCopyAction());
        parentMenu->addAction(packageActions->GetPasteAction());
        parentMenu->addAction(packageActions->GetDuplicateAction());
        parentMenu->addSeparator();
        parentMenu->addAction(packageActions->GetDeleteAction());
        parentMenu->addSeparator();
        parentMenu->addAction(packageActions->GetJumpToPrototypeAction());
        parentMenu->addAction(packageActions->GetFindPrototypeInstancesAction());
        parentMenu->addSeparator();
    }
}

void PreviewWidget::AddBgrColorMenuSection(QMenu* menu)
{
    using namespace DAVA;

    QMenu* bgrColorsMenu = new QMenu("Background Color");
    menu->addMenu(bgrColorsMenu);

    FieldDescriptor indexFieldDescr;
    indexFieldDescr.type = ReflectedTypeDB::Get<PreviewWidgetSettings>();
    indexFieldDescr.fieldName = FastName("backgroundColorIndex");

    FieldDescriptor colorsFieldDescr;
    colorsFieldDescr.type = ReflectedTypeDB::Get<PreviewWidgetSettings>();
    colorsFieldDescr.fieldName = DAVA::FastName("backgroundColors");

    PreviewWidgetSettings* settings = accessor->GetGlobalContext()->GetData<PreviewWidgetSettings>();
    const Vector<Color>& colors = settings->backgroundColors;
    for (DAVA::uint32 currentIndex = 0; currentIndex < colors.size(); ++currentIndex)
    {
        QtAction* action = new QtAction(accessor, QString("Background color %1").arg(currentIndex));
        action->SetStateUpdationFunction(QtAction::Icon, colorsFieldDescr, [currentIndex](const Any& v)
                                         {
                                             const Vector<Color>& colors = v.Cast<Vector<Color>>();
                                             Any color = colors[currentIndex];
                                             return color.Cast<QIcon>(QIcon());
                                         });

        action->SetStateUpdationFunction(QtAction::Checked, indexFieldDescr, [currentIndex](const Any& v)
                                         {
                                             return v.Cast<DAVA::uint32>(-1) == currentIndex;
                                         });
        connections.AddConnection(action, &QAction::triggered, [this, currentIndex]()
                                  {
                                      PreviewWidgetSettings* settings = accessor->GetGlobalContext()->GetData<PreviewWidgetSettings>();
                                      settings->backgroundColorIndex = currentIndex;
                                  });
        bgrColorsMenu->addAction(action);
    }
}

bool PreviewWidget::CanChangeTextInControl(const ControlNode* node) const
{
    using namespace DAVA;

    if (node == nullptr)
    {
        return false;
    }

    UIControl* control = node->GetControl();

    UITextComponent* textComponent = control->GetComponent<UITextComponent>();
    return textComponent != nullptr;
}

DAVA::List<ControlNode*> PreviewWidget::GetPathFromRoot(ControlNode* node)
{
    using namespace DAVA;

    const PackageNode* package = node->GetPackage();
    ImportedPackagesNode* importedRoot = package->GetImportedPackagesNode();
    PackageControlsNode* controlsRoot = package->GetPackageControlsNode();
    PackageControlsNode* prototypesRoot = package->GetPrototypes();
    StyleSheetsNode* styleSheetsRoot = package->GetStyleSheets();

    auto isRootControl = [&](const ControlNode* node) -> bool
    {
        PackageBaseNode* parent = node->GetParent();
        return (parent == controlsRoot || parent == prototypesRoot || parent == importedRoot || parent == styleSheetsRoot);
    };

    auto getParentNode = [&](const ControlNode* node) -> ControlNode*
    {
        return isRootControl(node) ? nullptr : dynamic_cast<ControlNode*>(node->GetParent());
    };

    List<ControlNode*> path = { node };

    for (ControlNode* nextNode = getParentNode(node);
         nextNode != nullptr;
         nextNode = getParentNode(nextNode))
    {
        path.push_front(nextNode);
    }

    path.push_front(nullptr);

    return path;
}

void PreviewWidget::InsertControlIntoLayer(LayerNode& layer, DAVA::List<ControlNode*>& pathFromRoot)
{
    DVASSERT(pathFromRoot.front() == layer.control);
    pathFromRoot.pop_front();

    if (pathFromRoot.empty() == true)
    {
        layer.enabled = true;
    }
    else
    {
        ControlNode* control = pathFromRoot.front();

        auto found = std::find_if(layer.subLayers.begin(), layer.subLayers.end(), [control](LayerNode& subNode)
                                  {
                                      return (subNode.control == control);
                                  });

        if (found != layer.subLayers.end())
        {
            InsertControlIntoLayer(*found, pathFromRoot);
        }
        else
        {
            LayerNode deeperLayer;
            deeperLayer.control = control;
            if (control != nullptr) // we are not on first level now
            {
                deeperLayer.leadingSpaces = "  " + layer.leadingSpaces;
            }
            InsertControlIntoLayer(deeperLayer, pathFromRoot);
            layer.subLayers.push_back(std::move(deeperLayer));
        }
    }
}

void PreviewWidget::InsertLayerIntoMenu(LayerNode& layer, QMenu* menu)
{
    using namespace DAVA;

    ControlNode* control = layer.control;
    if (control != nullptr)
    {
        QString text = QString::fromStdString(layer.leadingSpaces + control->GetName());
        QAction* action = new QAction(text, menu);
        action->setCheckable(true);
        action->setEnabled(layer.enabled);
        menu->addAction(action);

        DataContext* activeContext = accessor->GetActiveContext();
        DVASSERT(activeContext != nullptr);
        DocumentData* data = activeContext->GetData<DocumentData>();
        const SelectedNodes& selectedNodes = data->GetSelectedNodes();

        if (selectedNodes.find(control) != selectedNodes.end())
        {
            action->setChecked(true);
        }
        connect(action, &QAction::toggled, [this, control]() {
            systemsManager->SelectNode(control);
        });
    }

    layer.subLayers.sort([](const LayerNode& n1, const LayerNode& n2)
                         {
                             PackageBaseNode* parent = n1.control->GetParent();
                             DVASSERT(parent == n2.control->GetParent());
                             return parent->GetIndex(n1.control) < parent->GetIndex(n2.control);
                         });

    for (LayerNode& subLayer : layer.subLayers)
    {
        InsertLayerIntoMenu(subLayer, menu);
    }
}

void PreviewWidget::OnMouseReleased(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        DVASSERT(nodeToChangeTextOnMouseRelease == nullptr);
        ShowMenu(event);
    }

    if (nodeToChangeTextOnMouseRelease)
    {
        QPoint point = event->pos();
        DAVA::Vector2 davaPoint(point.x(), point.y());
        ControlNode* node = systemsManager->GetControlNodeAtPoint(davaPoint);
        if (node == nodeToChangeTextOnMouseRelease && CanChangeTextInControl(node))
        {
            requestChangeTextInNode.Emit(node);
        }
        nodeToChangeTextOnMouseRelease = nullptr;
    }
}

void PreviewWidget::OnMouseDBClick(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        QPoint point = event->pos();
        DAVA::Vector2 davaPoint(point.x(), point.y());
        nodeToChangeTextOnMouseRelease = systemsManager->GetControlNodeAtPoint(davaPoint);
    }
}

void PreviewWidget::OnMouseMove(QMouseEvent* event)
{
    DVASSERT(nullptr != event);
    rulerController->UpdateRulerMarkers(event->pos());
}

void PreviewWidget::OnDragEntered(QDragEnterEvent* event)
{
    auto mimeData = event->mimeData();
    if (mimeData->hasFormat("text/uri-list"))
    {
        bool canDropAnyFile = false;
        QStringList strList = mimeData->text().split("\n", QString::SkipEmptyParts);
        for (const QString& str : strList)
        {
            QUrl url(str);
            if (url.isLocalFile())
            {
                QString path = url.toLocalFile();
                canDropAnyFile |= DragNDropHelper::IsExtensionSupported(path) && DragNDropHelper::IsFileFromProject(accessor, path);
            }
        }

        if (canDropAnyFile)
        {
            droppingFile.Emit(true);
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }
    else
    {
        event->accept();
    }
}

void PreviewWidget::OnDragMoved(QDragMoveEvent* event)
{
    DVASSERT(nullptr != event);
    ProcessDragMoveEvent(event) ? event->accept() : event->ignore();
}

bool PreviewWidget::ProcessDragMoveEvent(QDropEvent* event)
{
    using namespace DAVA;

    DVASSERT(nullptr != event);
    auto mimeData = event->mimeData();
    if (mimeData->hasFormat("text/uri-list"))
    {
        //filter this format on drag entered
        return true;
    }
    else if (mimeData->hasFormat("text/plain") || mimeData->hasFormat(PackageMimeData::MIME_TYPE))
    {
        QPoint pos = event->pos();
        DAVA::Vector2 davaPos(pos.x(), pos.y());
        ControlNode* node = systemsManager->GetControlNodeAtPoint(davaPos);

        if (nullptr != node)
        {
            if (node->IsReadOnly())
            {
                return false;
            }
            else
            {
                if (mimeData->hasFormat(PackageMimeData::MIME_TYPE))
                {
                    const PackageMimeData* controlMimeData = DynamicTypeCheck<const PackageMimeData*>(mimeData);
                    const Vector<ControlNode*>& srcControls = controlMimeData->GetControls();
                    for (const auto& srcNode : srcControls)
                    {
                        if (srcNode == node)
                        {
                            return false;
                        }
                    }
                }
                return true;
            }
        }
        else
        {
            //root node will be added
            return true;
        }
    }
    return false;
}

void PreviewWidget::OnDragLeaved(QDragLeaveEvent*)
{
    droppingFile.Emit(false);
}

void PreviewWidget::OnDrop(QDropEvent* event)
{
    using namespace DAVA;

    droppingFile.Emit(false);
    DVASSERT(nullptr != event);
    auto mimeData = event->mimeData();
    if (mimeData->hasFormat("text/plain") || mimeData->hasFormat(PackageMimeData::MIME_TYPE))
    {
        Vector2 pos(event->pos().x(), event->pos().y());
        PackageBaseNode* node = systemsManager->GetControlNodeAtPoint(pos);
        auto action = event->dropAction();
        uint32 index = 0;
        if (node == nullptr)
        {
            DataContext* active = accessor->GetActiveContext();
            DVASSERT(active != nullptr);
            const DocumentData* data = active->GetData<DocumentData>();
            DVASSERT(data != nullptr);
            const PackageNode* package = data->GetPackageNode();
            node = DynamicTypeCheck<PackageBaseNode*>(package->GetPackageControlsNode());
            index = systemsManager->GetIndexOfNearestRootControl(pos);
        }
        else
        {
            index = node->GetCount();
        }

        invoker->Invoke(QEGlobal::DropIntoPackageNode.ID, mimeData, action, node, index, &pos);
    }
    else if (mimeData->hasFormat("text/uri-list"))
    {
        QStringList list = mimeData->text().split("\n", QString::SkipEmptyParts);
        emit OpenPackageFiles(list);
    }
    renderWidget->setFocus();
}

void PreviewWidget::OnKeyPressed(QKeyEvent* event)
{
    using namespace DAVA;

    if (event->isAutoRepeat())
    {
        return;
    }
    int key = event->key();
    if (key == Qt::Key_Enter || key == Qt::Key_Return)
    {
        DataContext* active = accessor->GetActiveContext();
        if (active == nullptr)
        {
            return;
        }
        DocumentData* data = active->GetData<DocumentData>();
        const SelectedNodes& selectedNodes = data->GetSelectedNodes();
        if (selectedNodes.size() == 1)
        {
            ControlNode* node = dynamic_cast<ControlNode*>(*selectedNodes.begin());
            if (CanChangeTextInControl(node))
            {
                requestChangeTextInNode.Emit(node);
            }
        }
    }
}

void PreviewWidget::OnTabBarContextMenuRequested(const QPoint& pos)
{
    using namespace DAVA;

    Vector<uint64> allIDs;
    accessor->ForEachContext([&allIDs](const DataContext& context) {
        allIDs.push_back(context.GetID());
    });

    //no tabs at all, do nothing
    if (allIDs.empty())
    {
        return;
    }

    QTabBar* tabBar = findChild<QTabBar*>();
    DVASSERT(tabBar != nullptr);

    int index = tabBar->tabAt(pos);
    if (index == -1)
    {
        return;
    }

    QVariant data = tabBar->tabData(index);
    DVASSERT(data.canConvert<uint64>());
    uint64 currentId = data.value<uint64>();

    QMenu menu(this);
    QAction* closeTabAction = new QAction(tr("Close tab"), &menu);
    QAction* closeOtherTabsAction = new QAction(tr("Close other tabs"), &menu);
    QAction* closeAllTabsAction = new QAction(tr("Close all tabs"), &menu);
    QAction* selectInFileSystemAction = new QAction(tr("Select in File System"), &menu);

    closeOtherTabsAction->setEnabled(allIDs.size() > 1);

    menu.addAction(closeTabAction);
    menu.addAction(closeOtherTabsAction);
    menu.addAction(closeAllTabsAction);
    menu.addSeparator();
    menu.addAction(selectInFileSystemAction);

    connect(closeAllTabsAction, &QAction::triggered, [this, allIDs]()
            {
                for (uint64 id : allIDs)
                {
                    requestCloseTab.Emit(id);
                }
            });

    connect(closeTabAction, &QAction::triggered, std::bind(&Signal<uint64>::Emit, &requestCloseTab, currentId));

    connect(closeOtherTabsAction, &QAction::triggered, [this, allIDs, currentId]()
            {
                for (uint64 id : allIDs)
                {
                    if (id != currentId)
                    {
                        requestCloseTab.Emit(id);
                    }
                }
            });

    connect(selectInFileSystemAction, &QAction::triggered, [this, currentId]() {
        QString filePath = accessor->GetContext(currentId)->GetData<DocumentData>()->GetPackageAbsolutePath();
        invoker->Invoke(QEGlobal::SelectFile.ID, filePath);
    });

    menu.exec(tabBar->mapToGlobal(pos));
}

void PreviewWidget::RegisterPackageActions(Interfaces::PackageActionsInterface* packageActions_)
{
    packageActions = packageActions_;
    renderWidget->addAction(packageActions->GetImportPackageAction());
    renderWidget->addAction(packageActions->GetCopyAction());
    renderWidget->addAction(packageActions->GetCutAction());
    renderWidget->addAction(packageActions->GetPasteAction());
    renderWidget->addAction(packageActions->GetDeleteAction());
    renderWidget->addAction(packageActions->GetDuplicateAction());
    renderWidget->addAction(packageActions->GetFindPrototypeInstancesAction());
    renderWidget->addAction(packageActions->GetJumpToPrototypeAction());
}

void PreviewWidget::UnregisterPackageActions()
{
    renderWidget->removeAction(packageActions->GetImportPackageAction());
    renderWidget->removeAction(packageActions->GetCopyAction());
    renderWidget->removeAction(packageActions->GetCutAction());
    renderWidget->removeAction(packageActions->GetPasteAction());
    renderWidget->removeAction(packageActions->GetDeleteAction());
    renderWidget->removeAction(packageActions->GetDuplicateAction());
    renderWidget->removeAction(packageActions->GetFindPrototypeInstancesAction());
    renderWidget->removeAction(packageActions->GetJumpToPrototypeAction());
    packageActions = nullptr;
}
