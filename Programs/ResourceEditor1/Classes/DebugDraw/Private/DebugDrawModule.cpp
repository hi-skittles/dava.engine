#include "Classes/DebugDraw/DebugDrawModule.h"
#include "Classes/DebugDraw/Private/DebugDrawSystem.h"

#include "Classes/Application/REGlobal.h"
#include "Classes/SceneManager/SceneData.h"

#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/Qt/Scene/System/ModifSystem.h"
#include "Classes/Qt/Scene/System/HoodSystem.h"
#include "Classes/Qt/Scene/System/WayEditSystem.h"
#include "Classes/Qt/Tools/HangingObjectsHeight/HangingObjectsHeight.h"
#include "Classes/Qt/Tools/ToolButtonWithWidget/ToolButtonWithWidget.h"
#include "Classes/Deprecated/SceneValidator.h"

#include <TArc/WindowSubSystem/QtAction.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/Utils/ModuleCollection.h>
#include <TArc/Controls/ComboBox.h>
#include <TArc/Controls/QtBoxLayouts.h>

#include <Reflection/ReflectionRegistrator.h>
#include <Logger/Logger.h>
#include <Scene3D/Scene.h>
#include <Scene3D/Systems/RenderUpdateSystem.h>

namespace DebugDrawDetail
{
bool IsCurrentType(const DAVA::Any& value, ResourceEditor::eSceneObjectType type)
{
    if (value.CanCast<ResourceEditor::eSceneObjectType>() == false)
    {
        return false;
    }

    return value.Cast<ResourceEditor::eSceneObjectType>() == type;
}
}

ENUM_DECLARE(ResourceEditor::eSceneObjectType)
{
    ENUM_ADD_DESCR(ResourceEditor::eSceneObjectType::ESOT_NONE, "Off");
    ENUM_ADD_DESCR(ResourceEditor::eSceneObjectType::ESOT_NO_COLISION, "No Collision");
    ENUM_ADD_DESCR(ResourceEditor::eSceneObjectType::ESOT_TREE, "Tree");
    ENUM_ADD_DESCR(ResourceEditor::eSceneObjectType::ESOT_BUSH, "Bush");
    ENUM_ADD_DESCR(ResourceEditor::eSceneObjectType::ESOT_FRAGILE_PROJ, "Fragile Proj");
    ENUM_ADD_DESCR(ResourceEditor::eSceneObjectType::ESOT_FRAGILE_PROJ_INV, "Fragile ^Proj");
    ENUM_ADD_DESCR(ResourceEditor::eSceneObjectType::ESOT_FALLING, "Falling");

    ENUM_ADD_DESCR(ResourceEditor::eSceneObjectType::ESOT_BUILDING, "Building");
    ENUM_ADD_DESCR(ResourceEditor::eSceneObjectType::ESOT_INVISIBLE_WALL, "Invisible Wall");
    ENUM_ADD_DESCR(ResourceEditor::eSceneObjectType::ESOT_SPEED_TREE, "Speed Tree");
    ENUM_ADD_DESCR(ResourceEditor::eSceneObjectType::ESOT_UNDEFINED_COLLISION, "Undefined Collision");
}

class DebugDrawData : public DAVA::TArc::DataNode
{
private:
    friend class DebugDrawModule;
    std::unique_ptr<DebugDrawSystem> debugDrawSystem;

    ResourceEditor::eSceneObjectType GetDebugDrawObject() const
    {
        return debugDrawSystem->GetRequestedObjectType();
    }

    void SetDebugDrawObject(ResourceEditor::eSceneObjectType type)
    {
        debugDrawSystem->SetRequestedObjectType(type);
    }

    DAVA::float32 GetHeightForHangingObjects() const
    {
        return debugDrawSystem->GetHangingObjectsHeight();
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(DebugDrawData, DAVA::TArc::DataNode)
    {
        DAVA::ReflectionRegistrator<DebugDrawData>::Begin()
        .Field("currentObject", &DebugDrawData::GetDebugDrawObject, &DebugDrawData::SetDebugDrawObject)
        .Field("heightForHangingObjects", &DebugDrawData::GetHeightForHangingObjects, nullptr)
        .End();
    }
};

void DebugDrawModule::OnContextCreated(DAVA::TArc::DataContext* context)
{
    SceneData* sceneData = context->GetData<SceneData>();
    SceneEditor2* scene = sceneData->GetScene().Get();
    DVASSERT(scene != nullptr);

    std::unique_ptr<DebugDrawData> debugDrawData = std::make_unique<DebugDrawData>();
    debugDrawData->debugDrawSystem.reset(new DebugDrawSystem(scene));
    scene->AddSystem(debugDrawData->debugDrawSystem.get(), 0);

    context->CreateData(std::move(debugDrawData));
}

void DebugDrawModule::OnContextDeleted(DAVA::TArc::DataContext* context)
{
    SceneData* sceneData = context->GetData<SceneData>();
    SceneEditor2* scene = sceneData->GetScene().Get();

    DebugDrawData* debugDrawData = context->GetData<DebugDrawData>();
    scene->RemoveSystem(debugDrawData->debugDrawSystem.get());
}

void DebugDrawModule::PostInit()
{
    using namespace DAVA;
    using namespace DAVA::TArc;

    UI* ui = GetUI();
    ContextAccessor* accessor = GetAccessor();

    fieldBinder.reset(new FieldBinder(accessor));

    QAction* collisionTypeMenuAction = new QAction("Collision Type", nullptr);
    QList<QString> upperMenuPath;
    upperMenuPath.push_back("Scene");

    InsertionParams upperMenuInsertion(InsertionParams::eInsertionMethod::AfterItem, "VisibilityCheckSystem");
    ActionPlacementInfo placementInfo(CreateMenuPoint(upperMenuPath, upperMenuInsertion));
    ui->AddAction(mainWindowKey, placementInfo, collisionTypeMenuAction);

    FieldDescriptor sceneFieldDescr;
    sceneFieldDescr.fieldName = DAVA::FastName(SceneData::scenePropertyName);
    sceneFieldDescr.type = DAVA::ReflectedTypeDB::Get<SceneData>();

    QList<QString> menuCollisionPath, menuScenePath;
    menuScenePath << "Scene";
    menuCollisionPath << menuScenePath << "Collision Type";

    //Create menu
    bool separatorInserted = false;
    for (DAVA::int32 i = ResourceEditor::eSceneObjectType::ESOT_NONE; i < ResourceEditor::eSceneObjectType::ESOT_COUNT; i++)
    {
        ResourceEditor::eSceneObjectType type = static_cast<ResourceEditor::eSceneObjectType>(i);
        QString actionName = GlobalEnumMap<ResourceEditor::eSceneObjectType>::Instance()->ToString(type);

        QtAction* action = new QtAction(accessor, actionName);

        FieldDescriptor fieldDescr;
        fieldDescr.fieldName = DAVA::FastName("currentObject");
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<DebugDrawData>();

        action->SetStateUpdationFunction(QtAction::Checked, fieldDescr, DAVA::Bind(&DebugDrawDetail::IsCurrentType, DAVA::_1, type));
        action->SetStateUpdationFunction(QtAction::Enabled, sceneFieldDescr, [](const DAVA::Any& v) {
            return v.IsEmpty() == false;
        });

        connections.AddConnection(action, &QAction::triggered, DAVA::Bind(&DebugDrawModule::ChangeObject, this, type));

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateMenuPoint(menuCollisionPath));

        ui->AddAction(mainWindowKey, placementInfo, action);

        if (separatorInserted == false)
        {
            separatorInserted = true;
            ui->AddAction(mainWindowKey, placementInfo, new QtActionSeparator("separator"));
        }
    }

    //switches with different lods
    {
        QIcon switchesWithDifferentLodsIcon = QIcon(":/QtIcons/switches_with_different_lods.png");
        QtAction* action = new QtAction(accessor, "SwitchesWithDifferentLods");
        action->setIcon(switchesWithDifferentLodsIcon);

        QToolButton* switchesWithDifferentLodsBtn = new QToolButton();
        switchesWithDifferentLodsBtn->setIcon(switchesWithDifferentLodsIcon);
        switchesWithDifferentLodsBtn->setAutoRaise(false);
        switchesWithDifferentLodsBtn->setToolTip("Switches with Different LODs");

        AttachWidgetToAction(action, switchesWithDifferentLodsBtn);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint("DebugDrawToolbar"));
        placementInfo.AddPlacementPoint(CreateMenuPoint(menuScenePath, upperMenuInsertion));

        ui->AddAction(mainWindowKey, placementInfo, action);
        connections.AddConnection(switchesWithDifferentLodsBtn, &QToolButton::clicked, DAVA::MakeFunction(this, &DebugDrawModule::OnSwitchWithDifferentLODs));
        connections.AddConnection(action, &QAction::triggered, DAVA::MakeFunction(this, &DebugDrawModule::OnSwitchWithDifferentLODs));
    }

    //hanging objects
    {
        QIcon hangingIcon = QIcon(":/QtIcons/hangingobjects.png");
        QtAction* action = new QtAction(accessor, "HangingObjects");
        action->setIcon(hangingIcon);

        ToolButtonWithWidget* hangingBtn = new ToolButtonWithWidget();
        hangingBtn->setIcon(hangingIcon);
        hangingBtn->setAutoRaise(false);
        hangingBtn->setToolTip("Hanging Objects");

        action->SetStateUpdationFunction(QtAction::Enabled, sceneFieldDescr, [hangingBtn](const DAVA::Any& v) {
            bool enabled = (v.IsEmpty() == false);
            hangingBtn->setEnabled(enabled);
            return enabled;
        });

        HangingObjectsHeight* hangingObjectsWidget = new HangingObjectsHeight(nullptr);
        QPointer<HangingObjectsHeight> closureWidget = hangingObjectsWidget;
        FieldDescriptor descr;
        descr.type = DAVA::ReflectedTypeDB::Get<DebugDrawData>();
        descr.fieldName = DAVA::FastName("heightForHangingObjects");
        fieldBinder->BindField(descr, [closureWidget](const DAVA::Any& height) {
            if (closureWidget != nullptr)
            {
                if (height.CanCast<DAVA::float32>())
                {
                    closureWidget->SetHeight(height.Cast<DAVA::float32>());
                    closureWidget->setEnabled(true);
                }
                else
                {
                    closureWidget->SetHeight(DebugDrawSystem::HANGING_OBJECTS_DEFAULT_HEIGHT);
                    closureWidget->setEnabled(false);
                }
            }
        });

        hangingObjectsWidget->SetHeight(DebugDrawSystem::HANGING_OBJECTS_DEFAULT_HEIGHT);

        hangingBtn->SetWidget(hangingObjectsWidget);

        AttachWidgetToAction(action, hangingBtn);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateToolbarPoint("DebugDrawToolbar"));
        placementInfo.AddPlacementPoint(CreateMenuPoint(menuScenePath, upperMenuInsertion));

        ui->AddAction(mainWindowKey, placementInfo, action);
        connections.AddConnection(hangingObjectsWidget, &HangingObjectsHeight::HeightChanged, DAVA::MakeFunction(this, &DebugDrawModule::OnHangingObjectsHeight));
        connections.AddConnection(hangingBtn, &ToolButtonWithWidget::clicked, DAVA::MakeFunction(this, &DebugDrawModule::OnHangingObjects));
        connections.AddConnection(action, &QAction::triggered, DAVA::MakeFunction(this, &DebugDrawModule::OnHangingObjects));
    }

    //scene objects tool bar
    {
        QtAction* action = new QtAction(accessor, "SceneObject");

        ComboBox::Params params(accessor, ui, mainWindowKey);
        params.fields[ComboBox::Fields::IsReadOnly] = "readOnly";
        params.fields[ComboBox::Fields::Value] = "currentObject";
        params.fields[ComboBox::Fields::Enumerator] = "";

        ControlProxy* control = new ComboBox(params, accessor, DAVA::Reflection::Create(DAVA::ReflectedObject(this)));
        AttachWidgetToAction(action, control);

        ActionPlacementInfo placementInfo(CreateToolbarPoint("sceneToolBar"));

        ui->AddAction(mainWindowKey, placementInfo, action);
    }

    ui->AddAction(mainWindowKey, ActionPlacementInfo(CreateMenuPoint(menuScenePath, upperMenuInsertion)), new QtActionSeparator("separatorDebugDrawBegin"));
}

void DebugDrawModule::OnHangingObjectsHeight(double value)
{
    if (IsDisabled())
    {
        return;
    }

    DAVA::TArc::DataContext* context = GetAccessor()->GetActiveContext();
    DebugDrawData* debugDrawData = context->GetData<DebugDrawData>();

    debugDrawData->debugDrawSystem->SetHangingObjectsHeight(static_cast<DAVA::float32>(value));
}

void DebugDrawModule::OnSwitchWithDifferentLODs()
{
    if (IsDisabled())
    {
        return;
    }

    DAVA::TArc::DataContext* context = GetAccessor()->GetActiveContext();
    DebugDrawData* debugDrawData = context->GetData<DebugDrawData>();

    bool isEnable = !debugDrawData->debugDrawSystem->SwithcesWithDifferentLODsModeEnabled();

    debugDrawData->debugDrawSystem->EnableSwithcesWithDifferentLODsMode(isEnable);

    if (isEnable)
    {
        SceneData* sceneData = context->GetData<SceneData>();
        SceneEditor2* scene = sceneData->GetScene().Get();

        DAVA::Set<DAVA::FastName> entitiNames;
        SceneValidator::FindSwitchesWithDifferentLODs(scene, entitiNames);

        DAVA::Set<DAVA::FastName>::iterator it = entitiNames.begin();
        DAVA::Set<DAVA::FastName>::iterator endIt = entitiNames.end();
        while (it != endIt)
        {
            DAVA::Logger::Info("Entity %s has different lods count.", it->c_str());
            ++it;
        }
    }
}

void DebugDrawModule::OnHangingObjects()
{
    if (IsDisabled())
    {
        return;
    }

    DAVA::TArc::DataContext* context = GetAccessor()->GetActiveContext();
    DebugDrawData* debugDrawData = context->GetData<DebugDrawData>();

    bool isEnable = !debugDrawData->debugDrawSystem->HangingObjectsModeEnabled();

    debugDrawData->debugDrawSystem->EnableHangingObjectsMode(isEnable);
}

void DebugDrawModule::ChangeObject(ResourceEditor::eSceneObjectType object)
{
    if (IsDisabled())
    {
        return;
    }

    DAVA::TArc::DataContext* context = GetAccessor()->GetActiveContext();
    DebugDrawData* debugDrawData = context->GetData<DebugDrawData>();

    debugDrawData->SetDebugDrawObject(object);
}

bool DebugDrawModule::IsDisabled() const
{
    return GetAccessor()->GetActiveContext() == nullptr;
}

ResourceEditor::eSceneObjectType DebugDrawModule::DebugDrawObject() const
{
    if (IsDisabled())
    {
        return ResourceEditor::eSceneObjectType::ESOT_NONE;
    }

    const DAVA::TArc::DataContext* context = GetAccessor()->GetActiveContext();
    const DebugDrawData* debugDrawData = context->GetData<DebugDrawData>();

    return debugDrawData->GetDebugDrawObject();
}

void DebugDrawModule::SetDebugDrawObject(ResourceEditor::eSceneObjectType type)
{
    if (IsDisabled())
    {
        return;
    }

    DAVA::TArc::DataContext* context = GetAccessor()->GetActiveContext();
    DebugDrawData* debugDrawData = context->GetData<DebugDrawData>();
    debugDrawData->SetDebugDrawObject(type);
}

DAVA_VIRTUAL_REFLECTION_IMPL(DebugDrawModule)
{
    DAVA::ReflectionRegistrator<DebugDrawModule>::Begin()
    .ConstructorByPointer()
    .Field("readOnly", &DebugDrawModule::IsDisabled, nullptr)
    .Field("currentObject", &DebugDrawModule::DebugDrawObject, &DebugDrawModule::SetDebugDrawObject)[DAVA::M::EnumT<ResourceEditor::eSceneObjectType>()]
    .End();
}

DECL_GUI_MODULE(DebugDrawModule);
