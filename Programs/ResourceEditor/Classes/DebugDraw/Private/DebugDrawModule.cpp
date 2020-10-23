#include "Classes/DebugDraw/DebugDrawModule.h"

#include "Classes/PropertyPanel/PropertyPanelCommon.h"
#include "Classes/PropertyPanel/CollisionTypeComponentExtensions.h"

#include "Classes/Qt/Tools/HangingObjectsHeight/HangingObjectsHeight.h"
#include "Classes/Qt/Tools/ToolButtonWithWidget/ToolButtonWithWidget.h"

#include <REPlatform/Scene/Systems/HoodSystem.h>
#include <REPlatform/Scene/Systems/ModifSystem.h>
#include <REPlatform/Scene/Systems/WayEditSystem.h>
#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/Systems/DebugDrawSystem.h>
#include <REPlatform/Scene/Components/CollisionTypeComponent.h>
#include <REPlatform/DataNodes/DebugDrawModuleData.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/Deprecated/SceneValidator.h>
#include <REPlatform/Deprecated/EditorConfig.h>
#include <REPlatform/Global/PropertyPanelInterface.h>

#include <TArc/WindowSubSystem/QtAction.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/Utils/ModuleCollection.h>
#include <TArc/Controls/ComboBox.h>
#include <TArc/Controls/QtBoxLayouts.h>

#include <Reflection/ReflectionRegistrator.h>
#include <Logger/Logger.h>
#include <Entity/ComponentManager.h>
#include <Scene3D/Scene.h>
#include <Scene3D/Systems/RenderUpdateSystem.h>

namespace DebugDrawDetail
{
bool IsCurrentType(const DAVA::Any& value, DAVA::int32 type)
{
    if (value.CanCast<DAVA::int32>() == false)
    {
        return false;
    }

    return value.Cast<DAVA::int32>() == type;
}
}

class DebugDrawData : public DAVA::TArcDataNode
{
private:
    friend class DebugDrawModule;
    std::unique_ptr<DAVA::DebugDrawSystem> debugDrawSystem;

    DAVA::int32 GetCollisionType() const
    {
        return debugDrawSystem->GetCollisionType();
    }

    void SetCollisionType(DAVA::int32 type)
    {
        debugDrawSystem->SetCollisionType(type);
    }

    DAVA::float32 GetHeightForHangingObjects() const
    {
        return debugDrawSystem->GetHangingObjectsHeight();
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(DebugDrawData, DAVA::TArcDataNode)
    {
        DAVA::ReflectionRegistrator<DebugDrawData>::Begin()
        .Field("collisionType", &DebugDrawData::GetCollisionType, &DebugDrawData::SetCollisionType)
        .Field("heightForHangingObjects", &DebugDrawData::GetHeightForHangingObjects, nullptr)
        .End();
    }
};

class CollisionTypePropertyPanelExtension : public DAVA::TArcDataNode
{
public:
    std::shared_ptr<DAVA::EditorComponentExtension> editorCreator;
    std::shared_ptr<DAVA::ChildCreatorExtension> childCreator;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(CollisionTypePropertyPanelExtension, DAVA::TArcDataNode)
    {
        DAVA::ReflectionRegistrator<DebugDrawData>::Begin()
        .End();
    }
};

DebugDrawModule::DebugDrawModule()
{
    using namespace DAVA;
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(CollisionTypeComponent);
}

void DebugDrawModule::OnContextCreated(DAVA::DataContext* context)
{
    DAVA::SceneData* sceneData = context->GetData<DAVA::SceneData>();
    DAVA::SceneEditor2* scene = sceneData->GetScene().Get();
    DVASSERT(scene != nullptr);

    std::unique_ptr<DebugDrawData> debugDrawData = std::make_unique<DebugDrawData>();
    debugDrawData->debugDrawSystem.reset(new DAVA::DebugDrawSystem(scene));
    scene->AddSystem(debugDrawData->debugDrawSystem.get(), 0);

    context->CreateData(std::move(debugDrawData));
}

void DebugDrawModule::OnContextDeleted(DAVA::DataContext* context)
{
    DAVA::SceneData* sceneData = context->GetData<DAVA::SceneData>();
    DAVA::SceneEditor2* scene = sceneData->GetScene().Get();

    DebugDrawData* debugDrawData = context->GetData<DebugDrawData>();
    scene->RemoveSystem(debugDrawData->debugDrawSystem.get());
}

void DebugDrawModule::OnInterfaceRegistered(const DAVA::Type* interfaceType)
{
    if (interfaceType == DAVA::Type::Instance<DAVA::PropertyPanelInterface>())
    {
        DAVA::PropertyPanelInterface* propertyPanel = QueryInterface<DAVA::PropertyPanelInterface>();

        CollisionTypePropertyPanelExtension* data = GetAccessor()->GetGlobalContext()->GetData<CollisionTypePropertyPanelExtension>();
        propertyPanel->RegisterExtension(data->childCreator);
        propertyPanel->RegisterExtension(data->editorCreator);
    }
}

void DebugDrawModule::OnBeforeInterfaceUnregistered(const DAVA::Type* interfaceType)
{
    if (interfaceType == DAVA::Type::Instance<DAVA::PropertyPanelInterface>())
    {
        DAVA::PropertyPanelInterface* propertyPanel = QueryInterface<DAVA::PropertyPanelInterface>();

        CollisionTypePropertyPanelExtension* data = GetAccessor()->GetGlobalContext()->GetData<CollisionTypePropertyPanelExtension>();
        propertyPanel->RegisterExtension(data->childCreator);
        propertyPanel->UnregisterExtension(data->editorCreator);
    }
}

void DebugDrawModule::PostInit()
{
    using namespace DAVA;

    UI* ui = GetUI();
    ContextAccessor* accessor = GetAccessor();

    fieldBinder.reset(new FieldBinder(accessor));

    accessor->GetGlobalContext()->CreateData(std::make_unique<DebugDrawModuleData>());

    CollisionTypePropertyPanelExtension* extensionData = new CollisionTypePropertyPanelExtension();
    extensionData->childCreator.reset(new PropertyPanel::CollisionTypeChildCreator());
    extensionData->editorCreator.reset(new PropertyPanel::CollisionTypeEditorCreator());
    accessor->GetGlobalContext()->CreateData(std::unique_ptr<DAVA::TArcDataNode>(extensionData));

    ProjectManagerData* projectData = accessor->GetGlobalContext()->GetData<ProjectManagerData>();
    FieldDescriptor projectPath;
    projectPath.type = DAVA::ReflectedTypeDB::Get<ProjectManagerData>();
    projectPath.fieldName = FastName(ProjectManagerData::ProjectPathProperty);
    fieldBinder->BindField(projectPath, [this](const DAVA::Any& path) {
        ContextAccessor* accessor = GetAccessor();
        ProjectManagerData* projectData = accessor->GetGlobalContext()->GetData<ProjectManagerData>();
        const EditorConfig* config = projectData->GetEditorConfig();
        CollisionTypesMap collisionTypes;
        const DAVA::Vector<std::pair<DAVA::int32, DAVA::String>>& collisionTypesVector = config->GetCollisionTypeMap("CollisionType");
        collisionTypes.values.reserve(collisionTypesVector.size() + 2);
        collisionTypes.values.emplace_back(CollisionTypeValues::COLLISION_TYPE_OFF, "Off");
        collisionTypes.values.insert(std::end(collisionTypes.values), std::begin(collisionTypesVector), std::end(collisionTypesVector));
        collisionTypes.values.emplace_back(CollisionTypeValues::COLLISION_TYPE_UNDEFINED, "Undefined collision");
        SetCollisionTypes(collisionTypes);

        CollisionTypesMap collisionTypesCrashed;
        collisionTypesCrashed.values = config->GetCollisionTypeMap("CollisionTypeCrashed");
        SetCollisionTypesCrashed(collisionTypesCrashed);
    });

    FieldDescriptor sceneFieldDescr;
    sceneFieldDescr.fieldName = DAVA::FastName(SceneData::scenePropertyName);
    sceneFieldDescr.type = DAVA::ReflectedTypeDB::Get<SceneData>();

    QList<QString> menuScenePath;
    menuScenePath << "Scene";
    InsertionParams upperMenuInsertion(InsertionParams::eInsertionMethod::AfterItem, "VisibilityCheckSystem");

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
        params.fields[ComboBox::Fields::Value] = "collisionType";
        params.fields[ComboBox::Fields::Enumerator] = "collisionTypes";

        ControlProxy* control = new ComboBox(params, accessor, DAVA::Reflection::Create(DAVA::ReflectedObject(this)));
        AttachWidgetToAction(action, control);

        ActionPlacementInfo placementInfo(CreateToolbarPoint("sceneToolBar"));

        ui->AddAction(mainWindowKey, placementInfo, action);
    }

    ui->AddAction(mainWindowKey, ActionPlacementInfo(CreateMenuPoint(menuScenePath, upperMenuInsertion)), new QtActionSeparator("separatorDebugDrawBegin"));

    DAVA::ComponentManager* cm = DAVA::GetEngineContext()->componentManager;
    cm->RegisterComponent(DAVA::Type::Instance<DAVA::CollisionTypeComponent>());
}

void DebugDrawModule::OnHangingObjectsHeight(double value)
{
    if (IsDisabled())
    {
        return;
    }

    DAVA::DataContext* context = GetAccessor()->GetActiveContext();
    DebugDrawData* debugDrawData = context->GetData<DebugDrawData>();

    debugDrawData->debugDrawSystem->SetHangingObjectsHeight(static_cast<DAVA::float32>(value));
}

void DebugDrawModule::OnSwitchWithDifferentLODs()
{
    if (IsDisabled())
    {
        return;
    }

    DAVA::DataContext* context = GetAccessor()->GetActiveContext();
    DebugDrawData* debugDrawData = context->GetData<DebugDrawData>();

    bool isEnable = !debugDrawData->debugDrawSystem->SwithcesWithDifferentLODsModeEnabled();

    debugDrawData->debugDrawSystem->EnableSwithcesWithDifferentLODsMode(isEnable);

    if (isEnable)
    {
        DAVA::SceneData* sceneData = context->GetData<DAVA::SceneData>();
        DAVA::SceneEditor2* scene = sceneData->GetScene().Get();

        DAVA::Set<DAVA::FastName> entitiNames;
        DAVA::SceneValidator::FindSwitchesWithDifferentLODs(scene, entitiNames);

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

    DAVA::DataContext* context = GetAccessor()->GetActiveContext();
    DebugDrawData* debugDrawData = context->GetData<DebugDrawData>();

    bool isEnable = !debugDrawData->debugDrawSystem->HangingObjectsModeEnabled();

    debugDrawData->debugDrawSystem->EnableHangingObjectsMode(isEnable);
}

bool DebugDrawModule::IsDisabled() const
{
    return GetAccessor()->GetActiveContext() == nullptr;
}

DAVA::int32 DebugDrawModule::GetCollisionType() const
{
    if (IsDisabled())
    {
        return 0;
    }

    const DAVA::DataContext* context = GetAccessor()->GetActiveContext();
    const DebugDrawData* debugDrawData = context->GetData<DebugDrawData>();

    return debugDrawData->GetCollisionType();
}

void DebugDrawModule::SetCollisionType(DAVA::int32 type)
{
    if (IsDisabled())
    {
        return;
    }

    DAVA::DataContext* context = GetAccessor()->GetActiveContext();
    DebugDrawData* debugDrawData = context->GetData<DebugDrawData>();
    debugDrawData->SetCollisionType(type);
}

const CollisionTypesMap& DebugDrawModule::GetCollisionTypes() const
{
    DAVA::DebugDrawModuleData* debugDrawModuleData = GetAccessor()->GetGlobalContext()->GetData<DAVA::DebugDrawModuleData>();
    const CollisionTypesMap& map = debugDrawModuleData->GetCollisionTypes();
    return map;
}

void DebugDrawModule::SetCollisionTypes(const CollisionTypesMap& map)
{
    DAVA::DebugDrawModuleData* debugDrawModuleData = GetAccessor()->GetGlobalContext()->GetData<DAVA::DebugDrawModuleData>();
    debugDrawModuleData->SetCollisionTypes(map);
}

const CollisionTypesMap& DebugDrawModule::GetCollisionTypesCrashed() const
{
    DAVA::DebugDrawModuleData* debugDrawModuleData = GetAccessor()->GetGlobalContext()->GetData<DAVA::DebugDrawModuleData>();
    const CollisionTypesMap& map = debugDrawModuleData->GetCollisionTypesCrashed();
    return map;
}

void DebugDrawModule::SetCollisionTypesCrashed(const CollisionTypesMap& map)
{
    DAVA::DebugDrawModuleData* debugDrawModuleData = GetAccessor()->GetGlobalContext()->GetData<DAVA::DebugDrawModuleData>();
    debugDrawModuleData->SetCollisionTypesCrashed(map);
}

DAVA_VIRTUAL_REFLECTION_IMPL(DebugDrawModule)
{
    DAVA::ReflectionRegistrator<DebugDrawModule>::Begin()
    .ConstructorByPointer()
    .Field("readOnly", &DebugDrawModule::IsDisabled, nullptr)
    .Field("collisionType", &DebugDrawModule::GetCollisionType, &DebugDrawModule::SetCollisionType)
    .Field("collisionTypes", &DebugDrawModule::GetCollisionTypes, &DebugDrawModule::SetCollisionTypes)
    .Field("collisionTypesCrashed", &DebugDrawModule::GetCollisionTypesCrashed, &DebugDrawModule::SetCollisionTypesCrashed)
    .End();
}

DECL_TARC_MODULE(DebugDrawModule);
