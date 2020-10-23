#include "Classes/Modification/ModificationModule.h"
#include "Classes/Modification/Private/ModificationData.h"


#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/Scene/Systems/CollisionSystem.h>
#include <REPlatform/DataNodes/SelectionData.h>

#include <TArc/Controls/DoubleSpinBox.h>
#include <TArc/Controls/Label.h>
#include <TArc/Controls/QtBoxLayouts.h>
#include <TArc/Core/ContextAccessor.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Qt/QtIcon.h>
#include <TArc/Utils/ModuleCollection.h>
#include <TArc/Utils/Utils.h>
#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/WindowSubSystem/QtAction.h>

#include <Reflection/ReflectedMeta.h>
#include <Reflection/ReflectedTypeDB.h>

namespace ModificationModuleDetails
{
const QString toolbarName = "Modifications Toolbar";
}

struct ModificationInternalData : public DAVA::TArcDataNode
{
    DAVA::DoubleSpinBox* xFieldControl = nullptr;
    DAVA::DoubleSpinBox* yFieldControl = nullptr;
    DAVA::DoubleSpinBox* zFieldControl = nullptr;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ModificationInternalData, DAVA::TArcDataNode)
    {
    }
};

void ModificationModule::PostInit()
{
    GetAccessor()->GetGlobalContext()->CreateData(std::make_unique<ModificationInternalData>());
    BindData();
    CreateToolbar();
    CreateActions();
}

void ModificationModule::BindData()
{
    using namespace DAVA;

    fieldBinder = std::make_unique<DAVA::FieldBinder>(GetAccessor());

    FieldDescriptor selectionField;
    selectionField.type = ReflectedTypeDB::Get<SelectionData>();
    selectionField.fieldName = FastName(SelectionData::selectionPropertyName);

    FieldDescriptor selectionBoxField;
    selectionBoxField.type = ReflectedTypeDB::Get<SelectionData>();
    selectionBoxField.fieldName = FastName(SelectionData::selectionBoxPropertyName);

    FieldDescriptor transformTypeField;
    transformTypeField.type = ReflectedTypeDB::Get<ModificationData>();
    transformTypeField.fieldName = FastName(ModificationData::transformTypeField);

    fieldBinder->BindField(transformTypeField, [this](const Any&) { RecalculateTransformableSelectionField(); });
    fieldBinder->BindField(selectionField, [this](const Any&) { RecalculateTransformableSelectionField(); });
    fieldBinder->BindField(selectionBoxField, [this](const Any&) { RecalculateTransformableSelectionField(); });
}

void ModificationModule::CreateToolbar()
{
    using namespace DAVA;
    using namespace ModificationModuleDetails;

    QList<QString> location = QList<QString>() << "View"
                                               << "Toolbars";
    InsertionParams after = { InsertionParams::eInsertionMethod::AfterItem, "Main Toolbar" };
    ActionPlacementInfo toolbarTogglePlacement(CreateMenuPoint(location, after));
    GetUI()->DeclareToolbar(DAVA::mainWindowKey, toolbarTogglePlacement, toolbarName);
}

void ModificationModule::CreateActions()
{
    using namespace DAVA;
    using namespace ModificationModuleDetails;

    ContextAccessor* accessor = GetAccessor();
    UI* ui = GetUI();

    FieldDescriptor sceneField;
    sceneField.type = ReflectedTypeDB::Get<SceneData>();
    sceneField.fieldName = FastName(SceneData::scenePropertyName);

    FieldDescriptor transformTypeField;
    transformTypeField.type = ReflectedTypeDB::Get<ModificationData>();
    transformTypeField.fieldName = FastName(ModificationData::transformTypeField);

    FieldDescriptor transformPivotField;
    transformPivotField.type = ReflectedTypeDB::Get<ModificationData>();
    transformPivotField.fieldName = FastName(ModificationData::transformPivotField);

    auto isSceneNotEmpty = [](const Any& value) -> Any
    {
        return value.CanCast<SceneData::TSceneType>() && value.Cast<SceneData::TSceneType>().Get() != nullptr;
    };

    auto isTrue = [](const Any& value) -> Any
    {
        return value.Get<bool>(false);
    };

    auto makeBindable = [](QtAction* action)
    {
        KeyBindableActionInfo info;
        info.blockName = "Modification";
        info.context = action->shortcutContext();
        info.defaultShortcuts << action->shortcuts();
        MakeActionKeyBindable(action, info);
    };

    QString lastSeparatorName = "modification last";

    // last separator in menu
    {
        QAction* lastSeparator = new QtActionSeparator(lastSeparatorName);
        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, "Place on landscape" }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, lastSeparator);
    }

    // action Select
    {
        const QString actionName = "Select";
        QtAction* action = new QtAction(accessor, actionName);
        action->setIcon(SharedIcon(":/QtIcons/modify_select.png"));
        action->setShortcutContext(Qt::WindowShortcut);
        action->SetStateUpdationFunction(QtAction::Enabled, sceneField, isSceneNotEmpty);
        action->SetStateUpdationFunction(QtAction::Checked, transformTypeField, [](const DAVA::Any& value) -> DAVA::Any
                                         {
                                             return value.CanCast<Selectable::TransformType>() && value.Cast<Selectable::TransformType>() == Selectable::TransformType::Disabled;
                                         });

        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::SetTransformType, this, Selectable::TransformType::Disabled));

        makeBindable(action);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, lastSeparatorName }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }

    // action Move
    {
        const QString actionName = "Move";
        QtAction* action = new QtAction(accessor, actionName);
        action->setIcon(SharedIcon(":/QtIcons/modify_move.png"));
        action->setShortcutContext(Qt::WindowShortcut);
        action->setShortcut(QKeySequence("Q"));
        action->SetStateUpdationFunction(QtAction::Enabled, sceneField, isSceneNotEmpty);
        action->SetStateUpdationFunction(QtAction::Checked, transformTypeField, [](const DAVA::Any& value) -> DAVA::Any
                                         {
                                             return value.CanCast<Selectable::TransformType>() && value.Cast<Selectable::TransformType>() == Selectable::TransformType::Translation;
                                         });

        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::SetTransformType, this, Selectable::TransformType::Translation));
        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::ForceUpdateXYZControls, this));

        makeBindable(action);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, lastSeparatorName }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }

    // action Rotate
    {
        const QString actionName = "Rotate";
        QtAction* action = new QtAction(accessor, actionName);
        action->setIcon(SharedIcon(":/QtIcons/modify_rotate.png"));
        action->setShortcutContext(Qt::WindowShortcut);
        action->setShortcut(QKeySequence("E"));
        action->SetStateUpdationFunction(QtAction::Enabled, sceneField, isSceneNotEmpty);
        action->SetStateUpdationFunction(QtAction::Checked, transformTypeField, [](const DAVA::Any& value) -> DAVA::Any
                                         {
                                             return value.CanCast<Selectable::TransformType>() && value.Cast<Selectable::TransformType>() == Selectable::TransformType::Rotation;
                                         });

        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::SetTransformType, this, Selectable::TransformType::Rotation));
        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::ForceUpdateXYZControls, this));

        makeBindable(action);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, lastSeparatorName }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }

    // action Scale
    {
        const QString actionName = "Scale";
        QtAction* action = new QtAction(accessor, actionName);
        action->setIcon(SharedIcon(":/QtIcons/modify_scale.png"));
        action->setShortcutContext(Qt::WindowShortcut);
        action->setShortcut(QKeySequence("R"));
        action->SetStateUpdationFunction(QtAction::Enabled, sceneField, isSceneNotEmpty);
        action->SetStateUpdationFunction(QtAction::Checked, transformTypeField, [](const DAVA::Any& value) -> DAVA::Any
                                         {
                                             return value.CanCast<Selectable::TransformType>() && value.Cast<Selectable::TransformType>() == Selectable::TransformType::Scale;
                                         });

        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::SetTransformType, this, Selectable::TransformType::Scale));
        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::ForceUpdateXYZControls, this));

        makeBindable(action);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, lastSeparatorName }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }

    // separator in menu & toolbar
    {
        QAction* separator = new QtActionSeparator("sep1");
        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, lastSeparatorName }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, separator);
    }

    // action Transform In Local Coordinates
    {
        const QString actionName = "Transform In Local Coordinates";
        QtAction* action = new QtAction(accessor, actionName);
        action->setIcon(SharedIcon(":/QtIcons/local_coordinates.png"));
        action->setShortcutContext(Qt::WindowShortcut);
        action->setShortcut(QKeySequence("G"));

        FieldDescriptor transformInLocalField;
        transformInLocalField.type = DAVA::ReflectedTypeDB::Get<ModificationData>();
        transformInLocalField.fieldName = DAVA::FastName(ModificationData::transformInLocalField);

        action->SetStateUpdationFunction(QtAction::Enabled, sceneField, isSceneNotEmpty);
        action->SetStateUpdationFunction(QtAction::Checked, transformInLocalField, isTrue);

        connections.AddConnection(action, &QAction::triggered, [this](bool value) { SetTransformInLocalCoordinates(value); });

        makeBindable(action);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, lastSeparatorName }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }

    // separator in menu & toolbar
    {
        QAction* separator = new QtActionSeparator("sep2");
        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, lastSeparatorName }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, separator);
    }

    // action Use Selection Center
    {
        const QString actionName = "Use Selection Center";
        QtAction* action = new QtAction(accessor, actionName);
        action->setIcon(SharedIcon(":/QtIcons/pivot_common.png"));
        action->setShortcutContext(Qt::WindowShortcut);
        action->SetStateUpdationFunction(QtAction::Enabled, sceneField, isSceneNotEmpty);
        action->SetStateUpdationFunction(QtAction::Checked, transformPivotField, [](const DAVA::Any& value) -> DAVA::Any
                                         {
                                             return value.CanCast<Selectable::TransformPivot>() && value.Cast<Selectable::TransformPivot>() == Selectable::TransformPivot::CommonCenter;
                                         });

        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::SetTransformPivot, this, Selectable::TransformPivot::CommonCenter));

        makeBindable(action);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }

    // action Use Pivot Point Center
    {
        const QString actionName = "Use Pivot Point Center";
        QtAction* action = new QtAction(accessor, actionName);
        action->setIcon(SharedIcon(":/QtIcons/pivot_center.png"));
        action->setShortcutContext(Qt::WindowShortcut);
        action->SetStateUpdationFunction(QtAction::Enabled, sceneField, isSceneNotEmpty);
        action->SetStateUpdationFunction(QtAction::Checked, transformPivotField, [](const DAVA::Any& value) -> DAVA::Any
                                         {
                                             return value.CanCast<Selectable::TransformPivot>() && value.Cast<Selectable::TransformPivot>() == Selectable::TransformPivot::ObjectCenter;
                                         });

        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::SetTransformPivot, this, Selectable::TransformPivot::ObjectCenter));

        makeBindable(action);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }

    // separator in toolbar
    {
        QAction* separator = new QtActionSeparator("sep3");
        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, separator);
    }

    // action Lock Transform
    {
        const QString actionName = "Lock Transform";
        QtAction* action = new QtAction(accessor, actionName);
        action->setIcon(SharedIcon(":/QtIcons/lock_add.png"));
        action->setShortcutContext(Qt::WindowShortcut);
        action->SetStateUpdationFunction(QtAction::Enabled, sceneField, isSceneNotEmpty);

        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::OnLockTransform, this));

        makeBindable(action);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, lastSeparatorName }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }

    // action Unlock Transform
    {
        const QString actionName = "Unlock Transform";
        QtAction* action = new QtAction(accessor, actionName);
        action->setIcon(SharedIcon(":/QtIcons/lock_delete.png"));
        action->setShortcutContext(Qt::WindowShortcut);
        action->SetStateUpdationFunction(QtAction::Enabled, sceneField, isSceneNotEmpty);

        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::OnUnlockTransform, this));

        makeBindable(action);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, lastSeparatorName }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }

    // separator in menu
    {
        QAction* separator = new QtActionSeparator("sep4");
        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, lastSeparatorName }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, separator);
    }

    FieldDescriptor singleModifyField;
    singleModifyField.type = DAVA::ReflectedTypeDB::Get<ModificationData>();
    singleModifyField.fieldName = DAVA::FastName(ModificationData::modifyingSingleItemField);

    // action Zero Pivot Point
    {
        const QString actionName = "Zero Pivot Point";
        QtAction* action = new QtAction(accessor, actionName);
        action->setIcon(SharedIcon(":/QtIcons/pivot_move_to_zero.png"));
        action->setShortcutContext(Qt::WindowShortcut);
        action->SetStateUpdationFunction(QtAction::Enabled, singleModifyField, isTrue);

        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::OnZeroPivotPoint, this));

        makeBindable(action);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, lastSeparatorName }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }

    // action Center Pivot Point
    {
        const QString actionName = "Center Pivot Point";
        QtAction* action = new QtAction(accessor, actionName);
        action->setIcon(SharedIcon(":/QtIcons/pivot_mote_to_center.png"));
        action->setShortcutContext(Qt::WindowShortcut);
        action->SetStateUpdationFunction(QtAction::Enabled, singleModifyField, isTrue);

        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::OnCenterPivotPoint, this));

        makeBindable(action);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, lastSeparatorName }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }

    // separator in menu & toolbar
    {
        QAction* separator = new QtActionSeparator("sep5");
        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        placementInfo.AddPlacementPoint(CreateMenuPoint("Edit", { InsertionParams::eInsertionMethod::BeforeItem, lastSeparatorName }));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, separator);
    }

    // action Manual Transform Mode - Absolute/Offset
    {
        const QString actionName = "Manual Transform Mode - Absolute/Offset";
        QtAction* action = new QtAction(accessor, actionName);

        QIcon icon;
        icon.addFile(":/QtIcons/move_absolute.png", QSize(), QIcon::Normal, QIcon::State::Off);
        icon.addFile(":/QtIcons/move_offset.png", QSize(), QIcon::Normal, QIcon::State::On);
        action->setIcon(icon);
        action->setShortcutContext(Qt::WindowShortcut);

        FieldDescriptor pivotModeField;
        pivotModeField.type = DAVA::ReflectedTypeDB::Get<ModificationData>();
        pivotModeField.fieldName = DAVA::FastName(ModificationData::pivotModeField);

        action->SetStateUpdationFunction(QtAction::Enabled, sceneField, isSceneNotEmpty);
        action->SetStateUpdationFunction(QtAction::Checked, pivotModeField, [](const DAVA::Any& value) -> DAVA::Any
                                         {
                                             return value.CanCast<EntityModificationSystem::PivotMode>() && value.Cast<EntityModificationSystem::PivotMode>() == EntityModificationSystem::PivotRelative;
                                         });

        connections.AddConnection(action, &QAction::triggered, [this](bool value)
                                  {
                                      GetModificationData()->SetPivotMode(value ? EntityModificationSystem::PivotRelative : EntityModificationSystem::PivotAbsolute);
                                  });

        makeBindable(action);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }

    DAVA::FieldDescriptor enableModificationWidgetField;
    enableModificationWidgetField.type = DAVA::ReflectedTypeDB::Get<ModificationData>();
    enableModificationWidgetField.fieldName = DAVA::FastName(ModificationData::manualModificationEnabledField);

    // manual modification bar
    {
        QWidget* bar = CreateManualModificationControlBar();

        const QString actionName = "Manual Modification Bar";
        QtAction* action = new QtAction(accessor, actionName);
        AttachWidgetToAction(action, bar);

        action->SetStateUpdationFunction(QtAction::Enabled, enableModificationWidgetField, isTrue);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }

    // action Reset Transform
    {
        const QString actionName = "Reset Transform";
        QtAction* action = new QtAction(accessor, actionName);
        action->setIcon(SharedIcon(":/QtIcons/ccancel.png"));
        action->setShortcutContext(Qt::WindowShortcut);
        action->SetStateUpdationFunction(QtAction::Enabled, enableModificationWidgetField, isTrue);
        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::OnResetTransform, this));
        connections.AddConnection(action, &QAction::triggered, Bind(&ModificationModule::ForceUpdateXYZControls, this));

        makeBindable(action);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolbarName));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, action);
    }
}

QWidget* ModificationModule::CreateManualModificationControlBar()
{
    using namespace DAVA;

    ContextAccessor* accessor = GetAccessor();
    UI* ui = GetUI();

    Reflection reflection = DAVA::Reflection::Create(DAVA::ReflectedObject(this));

    QWidget* widget = new QWidget;

    QtHBoxLayout* horizontalLayout = new QtHBoxLayout(widget);
    horizontalLayout->setSpacing(2);
    horizontalLayout->setContentsMargins(2, 1, 2, 1);

    {
        Label::Params p(accessor, ui, DAVA::mainWindowKey);
        p.fields[Label::Fields::IsVisible].BindConstValue(true);
        p.fields[Label::Fields::Text] = ModificationModule::xLabelField;

        Label* xLabel = new Label(p, accessor, reflection, widget);
        horizontalLayout->AddControl(xLabel);
    }

    {
        DoubleSpinBox::Params p(accessor, ui, DAVA::mainWindowKey);
        p.fields[DoubleSpinBox::Fields::IsVisible].BindConstValue(true);
        p.fields[DoubleSpinBox::Fields::IsEnabled] = ModificationModule::manualModificationEnabledField;
        p.fields[DoubleSpinBox::Fields::ShowSpinArrows] = ModificationModule::manualModificationArrowsEnabledField;
        p.fields[DoubleSpinBox::Fields::Value] = ModificationModule::xValueField;

        DoubleSpinBox* xAxisSpinBox = new DoubleSpinBox(p, accessor, reflection, widget);
        xAxisSpinBox->ToWidgetCast()->setMinimumSize(QSize(80, 0));
        horizontalLayout->AddControl(xAxisSpinBox);
        GetModificationInternalData()->xFieldControl = xAxisSpinBox;
    }

    {
        Label::Params p(accessor, ui, DAVA::mainWindowKey);
        p.fields[Label::Fields::IsVisible] = ModificationModule::scaleModeOffField;
        p.fields[Label::Fields::Text].BindConstValue("Y:");

        Label* yLabel = new Label(p, accessor, reflection, widget);
        horizontalLayout->AddControl(yLabel);
    }

    {
        DoubleSpinBox::Params p(accessor, ui, DAVA::mainWindowKey);
        p.fields[DoubleSpinBox::Fields::IsVisible] = ModificationModule::scaleModeOffField;
        p.fields[DoubleSpinBox::Fields::IsEnabled] = ModificationModule::manualModificationEnabledField;
        p.fields[DoubleSpinBox::Fields::ShowSpinArrows] = ModificationModule::manualModificationArrowsEnabledField;
        p.fields[DoubleSpinBox::Fields::Value] = ModificationModule::yValueField;

        DoubleSpinBox* yAxisSpinBox = new DoubleSpinBox(p, accessor, reflection, widget);
        yAxisSpinBox->ToWidgetCast()->setMinimumSize(QSize(80, 0));
        horizontalLayout->AddControl(yAxisSpinBox);
        GetModificationInternalData()->yFieldControl = yAxisSpinBox;
    }

    {
        Label::Params p(accessor, ui, DAVA::mainWindowKey);
        p.fields[Label::Fields::IsVisible] = ModificationModule::scaleModeOffField;
        p.fields[Label::Fields::Text].BindConstValue("Z:");

        Label* yLabel = new Label(p, accessor, reflection, widget);
        horizontalLayout->AddControl(yLabel);
    }

    {
        DoubleSpinBox::Params p(accessor, ui, DAVA::mainWindowKey);
        p.fields[DoubleSpinBox::Fields::IsVisible] = ModificationModule::scaleModeOffField;
        p.fields[DoubleSpinBox::Fields::IsEnabled] = ModificationModule::manualModificationEnabledField;
        p.fields[DoubleSpinBox::Fields::ShowSpinArrows] = ModificationModule::manualModificationArrowsEnabledField;
        p.fields[DoubleSpinBox::Fields::Value] = ModificationModule::zValueField;

        DoubleSpinBox* zAxisSpinBox = new DoubleSpinBox(p, accessor, reflection, widget);
        zAxisSpinBox->ToWidgetCast()->setMinimumSize(QSize(80, 0));
        horizontalLayout->AddControl(zAxisSpinBox);
        GetModificationInternalData()->zFieldControl = zAxisSpinBox;
    }

    return widget;
}

void ModificationModule::OnContextCreated(DAVA::DataContext* context)
{
    DAVA::SceneData* sceneData = context->GetData<DAVA::SceneData>();
    DAVA::SceneEditor2* scene = sceneData->GetScene().Get();
    DVASSERT(scene != nullptr);

    std::unique_ptr<ModificationData> modificationData = std::make_unique<ModificationData>();
    modificationData->modificationSystem = scene->GetSystem<DAVA::EntityModificationSystem>();
    context->CreateData(std::move(modificationData));
}

DAVA::SceneEditor2* ModificationModule::GetCurrentScene() const
{
    const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    if (ctx != nullptr)
    {
        DAVA::SceneData* data = ctx->GetData<DAVA::SceneData>();
        if (data != nullptr)
        {
            return data->GetScene().Get();
        }
    }

    return nullptr;
}

const DAVA::SelectableGroup& ModificationModule::GetCurrentSelection() const
{
    DAVA::SelectionData* selectionData = GetAccessor()->GetActiveContext()->GetData<DAVA::SelectionData>();
    if (selectionData != nullptr)
    {
        return selectionData->GetSelection();
    }

    static DAVA::SelectableGroup emptyGroup;
    return emptyGroup;
}

ModificationData* ModificationModule::GetModificationData() const
{
    const DAVA::DataContext* context = GetAccessor()->GetActiveContext();
    return (context != nullptr) ? context->GetData<ModificationData>() : nullptr;
}

void ModificationModule::RecalculateTransformableSelectionField()
{
    using namespace DAVA;

    ModificationData* modificationData = GetModificationData();
    if (modificationData != nullptr)
    {
        SceneEditor2* scene = GetCurrentScene();
        DVASSERT(scene != nullptr);

        SceneCollisionSystem* collisionSystem = scene->GetSystem<SceneCollisionSystem>();
        DVASSERT(collisionSystem != nullptr);

        const SelectableGroup& selection = GetCurrentSelection();
        DAVA::Vector<Selectable> tranformableObjects;
        tranformableObjects.reserve(selection.GetSize());

        for (const Selectable& item : selection.GetContent())
        {
            if (item.SupportsTransformType(modificationData->GetTransformType()))
            {
                Selectable obj(item.GetContainedObject());
                obj.SetBoundingBox(collisionSystem->GetUntransformedBoundingBox(item.GetContainedObject()));
                tranformableObjects.push_back(obj);
            }
        }

        SelectableGroup transformableSelection;
        transformableSelection.Add(tranformableObjects);
        transformableSelection.RebuildIntegralBoundingBox();

        modificationData->modificationSystem->SetTransformableSelection(transformableSelection);
    }
}

void ModificationModule::SetTransformType(DAVA::Selectable::TransformType transformType)
{
    GetModificationData()->SetTransformType(transformType);
}

void ModificationModule::SetTransformPivot(DAVA::Selectable::TransformPivot transformPivot)
{
    GetModificationData()->SetTransformPivot(transformPivot);
}

void ModificationModule::SetTransformInLocalCoordinates(bool value)
{
    GetModificationData()->SetTransformInLocalCoordinates(value);
}

void ModificationModule::OnResetTransform()
{
    GetModificationData()->modificationSystem->ResetTransform(GetCurrentSelection());
}

void ModificationModule::OnLockTransform()
{
    GetModificationData()->modificationSystem->LockTransform(GetCurrentSelection(), true);
}

void ModificationModule::OnUnlockTransform()
{
    GetModificationData()->modificationSystem->LockTransform(GetCurrentSelection(), false);
}

void ModificationModule::OnCenterPivotPoint()
{
    GetModificationData()->modificationSystem->MovePivotCenter(GetCurrentSelection());
}

void ModificationModule::OnZeroPivotPoint()
{
    GetModificationData()->modificationSystem->MovePivotZero(GetCurrentSelection());
}

bool ModificationModule::IsModificationEnabled() const
{
    ModificationData* data = GetModificationData();
    return (data != nullptr) ? data->GetManualModificationEnabled() : false;
}

bool ModificationModule::IsModificationArrowsEnabled() const
{
    ModificationData* data = GetModificationData();
    return (data != nullptr) ? data->GetManualModificationArrowsEnabled() : false;
}

bool ModificationModule::IsScaleModeOff() const
{
    ModificationData* data = GetModificationData();
    return (data != nullptr) ? data->GetScaleModeOff() : true;
}

DAVA::String ModificationModule::GetLabelX() const
{
    ModificationData* data = GetModificationData();
    return (data != nullptr) ? data->GetXLabelText() : "X:";
}

DAVA::Any ModificationModule::GetValueX() const
{
    static DAVA::Any emptyString = DAVA::String("");
    ModificationData* data = GetModificationData();
    return (data != nullptr) ? data->GetValueX() : emptyString;
}

DAVA::Any ModificationModule::GetValueY() const
{
    static DAVA::Any emptyString = DAVA::String("");
    ModificationData* data = GetModificationData();
    return (data != nullptr) ? data->GetValueY() : emptyString;
}

DAVA::Any ModificationModule::GetValueZ() const
{
    static DAVA::Any emptyString = DAVA::String("");
    ModificationData* data = GetModificationData();
    return (data != nullptr) ? data->GetValueZ() : emptyString;
}

void ModificationModule::SetValueX(const DAVA::Any& value)
{
    ModificationData* data = GetModificationData();
    if (data != nullptr)
    {
        data->SetValueX(value);
    }
}

void ModificationModule::SetValueY(const DAVA::Any& value)
{
    ModificationData* data = GetModificationData();
    if (data != nullptr)
    {
        data->SetValueY(value);
    }
}

void ModificationModule::SetValueZ(const DAVA::Any& value)
{
    ModificationData* data = GetModificationData();
    if (data != nullptr)
    {
        data->SetValueZ(value);
    }
}

ModificationInternalData* ModificationModule::GetModificationInternalData() const
{
    const DAVA::DataContext* context = GetAccessor()->GetGlobalContext();
    return (context != nullptr) ? context->GetData<ModificationInternalData>() : nullptr;
}

void ModificationModule::ForceUpdateXYZControls()
{
    ModificationInternalData* data = GetModificationInternalData();
    data->xFieldControl->ForceUpdate();
    data->yFieldControl->ForceUpdate();
    data->zFieldControl->ForceUpdate();
}

DAVA_VIRTUAL_REFLECTION_IMPL(ModificationModule)
{
    DAVA::ReflectionRegistrator<ModificationModule>::Begin()
    .ConstructorByPointer()
    .Field(manualModificationEnabledField, &ModificationModule::IsModificationEnabled, nullptr)
    .Field(manualModificationArrowsEnabledField, &ModificationModule::IsModificationArrowsEnabled, nullptr)
    .Field(scaleModeOffField, &ModificationModule::IsScaleModeOff, nullptr)
    .Field(xLabelField, &ModificationModule::GetLabelX, nullptr)
    .Field(xValueField, &ModificationModule::GetValueX, &ModificationModule::SetValueX)[DAVA::M::FloatNumberAccuracy(3)]
    .Field(yValueField, &ModificationModule::GetValueY, &ModificationModule::SetValueY)[DAVA::M::FloatNumberAccuracy(3)]
    .Field(zValueField, &ModificationModule::GetValueZ, &ModificationModule::SetValueZ)[DAVA::M::FloatNumberAccuracy(3)]
    .End();
}

const char* ModificationModule::manualModificationEnabledField = "isManualModificationEnabled";
const char* ModificationModule::manualModificationArrowsEnabledField = "isManualModificationArrowsEnabled";
const char* ModificationModule::scaleModeOffField = "scaleModeOff";
const char* ModificationModule::xLabelField = "xLabel";
const char* ModificationModule::xValueField = "xValue";
const char* ModificationModule::yValueField = "yValue";
const char* ModificationModule::zValueField = "zValue";

DECL_TARC_MODULE(ModificationModule);
