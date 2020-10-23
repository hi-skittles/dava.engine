#include "Classes/UserNodeModule/UserNodeModule.h"
#include "Classes/UserNodeModule/Private/UserNodeSystem.h"
#include "Classes/UserNodeModule/Private/UserNodeData.h"

#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/SceneTree/CreateEntitySupport.h"

#include <TArc/Utils/ModuleCollection.h>
#include <TArc/Utils/Utils.h>
#include <TArc/WindowSubSystem/QtAction.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/WindowSubSystem/ActionUtils.h>

#include <Entity/ComponentUtils.h>
#include <FileSystem/FilePath.h>
#include <Functional/Function.h>
#include <Logger/Logger.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Scene3D/Components/UserComponent.h>
#include <Scene3D/Entity.h>

namespace UserNodeModuleDetail
{
class UserNodeEntityCreator : public SimpleEntityCreator
{
    using TBase = SimpleEntityCreator;

public:
    static DAVA::RefPtr<DAVA::Entity> CreateEntity()
    {
        DAVA::RefPtr<DAVA::Entity> sceneNode(new DAVA::Entity());
        sceneNode->AddComponent(new DAVA::UserComponent());
        sceneNode->SetName(ResourceEditor::USER_NODE_NAME);

        return sceneNode;
    }

    UserNodeEntityCreator()
        : TBase(eMenuPointOrder::USER_NODE_ENITY, DAVA::TArc::SharedIcon(":/QtIcons/user_object.png"),
                QStringLiteral("User Node"), DAVA::MakeFunction(&UserNodeEntityCreator::CreateEntity))
    {
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(UserNodeEntityCreator, TBase)
    {
        DAVA::ReflectionRegistrator<UserNodeEntityCreator>::Begin()
        .ConstructorByPointer()
        .End();
    }
};
} // namespace UserNodeModuleDetail

DAVA::FilePath UserNodeModule::GetBotSpawnPath()
{
    return "~res:/ResourceEditor/3DObjects/Botspawn/tanksbox.sc2";
}

UserNodeModule::UserNodeModule()
{
    DAVA_REFLECTION_REGISTER_CUSTOM_PERMANENT_NAME(UserNodeModuleDetail::UserNodeEntityCreator, "UserNodeEntityCreator");
}

void UserNodeModule::PostInit()
{
    using namespace DAVA;
    using namespace DAVA::TArc;

    QtAction* action = new QtAction(GetAccessor(), QIcon(":/QtIcons/user_object.png"), QString("Custom UserNode Drawing Enabled"));
    { // checked-unchecked and text
        FieldDescriptor fieldDescr;
        fieldDescr.fieldName = DAVA::FastName(UserNodeData::drawingEnabledPropertyName);
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<UserNodeData>();
        action->SetStateUpdationFunction(QtAction::Checked, fieldDescr, [](const DAVA::Any& value) -> DAVA::Any {
            return value.Get<bool>(false);
        });
        action->SetStateUpdationFunction(QtAction::Text, fieldDescr, [](const DAVA::Any& value) -> DAVA::Any {
            if (value.Get<bool>(false))
                return DAVA::String("Custom UserNode Drawing Enabled");
            return DAVA::String("Custom UserNode Drawing Disabled");
        });
    }

    { // enabled/disabled state
        FieldDescriptor fieldDescr;
        fieldDescr.fieldName = DAVA::FastName(SceneData::scenePropertyName);
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<SceneData>();
        action->SetStateUpdationFunction(QtAction::Enabled, fieldDescr, [](const DAVA::Any& value) -> DAVA::Any {
            return value.CanCast<SceneData::TSceneType>() && value.Cast<SceneData::TSceneType>().Get() != nullptr;
        });
    }

    connections.AddConnection(action, &QAction::triggered, DAVA::Bind(&UserNodeModule::ChangeDrawingState, this));

    ActionPlacementInfo placementInfo;
    placementInfo.AddPlacementPoint(CreateStatusbarPoint(true, 0, { InsertionParams::eInsertionMethod::AfterItem, "actionShowStaticOcclusion" }));

    GetUI()->AddAction(DAVA::TArc::mainWindowKey, placementInfo, action);

    { //handle visibility of HUD
        FieldDescriptor fieldDescriptor(ReflectedTypeDB::Get<SceneData>(), FastName(SceneData::sceneHUDVisiblePropertyName));
        fieldBinder.reset(new FieldBinder(GetAccessor()));
        fieldBinder->BindField(fieldDescriptor, MakeFunction(this, &UserNodeModule::OnHUDVisibilityChanged));
    }
}

void UserNodeModule::OnContextCreated(DAVA::TArc::DataContext* context)
{
    SceneData* sceneData = context->GetData<SceneData>();
    SceneEditor2* scene = sceneData->GetScene().Get();
    DVASSERT(scene != nullptr);

    std::unique_ptr<UserNodeData> userData = std::make_unique<UserNodeData>();
    userData->system.reset(new UserNodeSystem(scene, GetBotSpawnPath()));
    userData->system->DisableSystem();
    scene->AddSystem(userData->system.get(), DAVA::ComponentUtils::MakeMask<DAVA::UserComponent>(), DAVA::Scene::SCENE_SYSTEM_REQUIRE_PROCESS);

    context->CreateData(std::move(userData));
}

void UserNodeModule::OnContextDeleted(DAVA::TArc::DataContext* context)
{
    using namespace DAVA::TArc;

    SceneData* sceneData = context->GetData<SceneData>();
    SceneEditor2* scene = sceneData->GetScene().Get();

    UserNodeData* userData = context->GetData<UserNodeData>();
    scene->RemoveSystem(userData->system.get());
}

void UserNodeModule::ChangeDrawingState()
{
    DAVA::TArc::DataContext* context = GetAccessor()->GetActiveContext();
    UserNodeData* moduleData = context->GetData<UserNodeData>();

    bool enabled = moduleData->IsDrawingEnabled();
    moduleData->SetDrawingEnabled(!enabled);
}

void UserNodeModule::OnHUDVisibilityChanged(const DAVA::Any& hudVisibilityValue)
{
    DAVA::TArc::DataContext* context = GetAccessor()->GetActiveContext();
    if (context != nullptr)
    {
        UserNodeData* userData = context->GetData<UserNodeData>();
        DVASSERT(userData != nullptr);

        bool isHudVisible = hudVisibilityValue.Cast<bool>(false);
        userData->system->SetVisible(isHudVisible);
    }
}

DAVA_VIRTUAL_REFLECTION_IMPL(UserNodeModule)
{
    DAVA::ReflectionRegistrator<UserNodeModule>::Begin()
    .ConstructorByPointer()
    .End();
}

DECL_GUI_MODULE(UserNodeModule);
