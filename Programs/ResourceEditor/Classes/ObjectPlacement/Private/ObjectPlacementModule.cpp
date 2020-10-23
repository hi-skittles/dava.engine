#include "Classes/ObjectPlacement/ObjectPlacementModule.h"
#include "Classes/ObjectPlacement/Private/ObjectPlacementData.h"
#include "Classes/ObjectPlacement/Private/ObjectPlacementSystem.h"

#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/Systems/ModifSystem.h>
#include <REPlatform/DataNodes/SceneData.h>

#include <TArc/Utils/ModuleCollection.h>
#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/WindowSubSystem/QtAction.h>

#include <Entity/ComponentUtils.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/RenderComponent.h>

void ObjectPlacementModule::OnContextCreated(DAVA::DataContext* context)
{
    DAVA::SceneData* sceneData = context->GetData<DAVA::SceneData>();
    DAVA::SceneEditor2* scene = sceneData->GetScene().Get();

    std::unique_ptr<ObjectPlacementData> objectPlacementData = std::make_unique<ObjectPlacementData>();

    objectPlacementData->objectPlacementSystem.reset(new ObjectPlacementSystem(scene));
    scene->AddSystem(objectPlacementData->objectPlacementSystem.get(),
                     DAVA::ComponentUtils::MakeMask<DAVA::RenderComponent>(),
                     DAVA::Scene::SCENE_SYSTEM_REQUIRE_PROCESS);

    context->CreateData(std::move(objectPlacementData));
}

void ObjectPlacementModule::OnContextDeleted(DAVA::DataContext* context)
{
    DAVA::SceneData* sceneData = context->GetData<DAVA::SceneData>();
    DAVA::SceneEditor2* scene = sceneData->GetScene().Get();

    ObjectPlacementData* objectPlacementData = context->GetData<ObjectPlacementData>();
    scene->RemoveSystem(objectPlacementData->objectPlacementSystem.get());
}

void ObjectPlacementModule::PostInit()
{
    const QString toolBarName("Main Toolbar");
    const QString editMenuName("Edit");

    const QString centerPivotPointName("actionCenterPivotPoint");

    using namespace DAVA;

    ContextAccessor* accessor = GetAccessor();
    UI* ui = GetUI();
    FieldDescriptor sceneDataFieldDescr;
    sceneDataFieldDescr.fieldName = DAVA::FastName(SceneData::scenePropertyName);
    sceneDataFieldDescr.type = DAVA::ReflectedTypeDB::Get<SceneData>();

    FieldDescriptor landscapeSnapFieldDescr;
    landscapeSnapFieldDescr.fieldName = DAVA::FastName(ObjectPlacementData::snapToLandscapePropertyName);
    landscapeSnapFieldDescr.type = DAVA::ReflectedTypeDB::Get<ObjectPlacementData>();

    // Place on landscape
    {
        QtAction* action = new QtAction(accessor, QIcon(":/QtIcons/modify_placeonland.png"), QString("Place on landscape"));
        { // enable/disable

            action->SetStateUpdationFunction(QtAction::Enabled, sceneDataFieldDescr, [](const DAVA::Any& value) -> DAVA::Any {
                return value.CanCast<SceneData::TSceneType>() && value.Cast<SceneData::TSceneType>().Get() != nullptr;
            });
        }
        action->setShortcuts({ QKeySequence(Qt::Key_P) });
        action->setShortcutContext(Qt::WindowShortcut);
        connections.AddConnection(action, &QAction::triggered, DAVA::MakeFunction(this, &ObjectPlacementModule::OnPlaceOnLandscape));
        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateMenuPoint(MenuItems::menuEdit,
                                                        { InsertionParams::eInsertionMethod::AfterItem, centerPivotPointName }));
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolBarName));

        ui->AddAction(mainWindowKey, placementInfo, action);
    }

    // Snap to landscape
    {
        QtAction* action = new QtAction(accessor, QIcon(":/QtIcons/modify_snaptoland.png"), "Enable snap to landscape");
        { // check/uncheck
            action->SetStateUpdationFunction(QtAction::Checked, landscapeSnapFieldDescr, [](const DAVA::Any& value) -> DAVA::Any {
                return value.Get<bool>(false);
            });
        }

        { // enable/disable
            action->SetStateUpdationFunction(QtAction::Enabled, sceneDataFieldDescr, [](const DAVA::Any& value) -> DAVA::Any {
                return value.CanCast<SceneData::TSceneType>() && value.Cast<SceneData::TSceneType>().Get() != nullptr;
            });
        }

        { // tooltip text
            action->SetStateUpdationFunction(QtAction::Text, landscapeSnapFieldDescr, [](const DAVA::Any& value) -> DAVA::Any {
                bool checked = value.Get<bool>(false);
                if (checked)
                    return DAVA::String("Disable snap to landscape");
                return DAVA::String("Enable snap to landscape");
            });
        }

        connections.AddConnection(action, &QAction::triggered, DAVA::MakeFunction(this, &ObjectPlacementModule::OnSnapToLandscape));
        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateMenuPoint(MenuItems::menuEdit,
                                                        { InsertionParams::eInsertionMethod::AfterItem, "Place on landscape" }));
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolBarName));

        ui->AddAction(mainWindowKey, placementInfo, action);
    }

    // Place and align
    {
        QtAction* action = new QtAction(accessor, QIcon(":/QtIcons/modify_placeonobj.png"), QString("Place and align"));
        { // enable/disable
            action->SetStateUpdationFunction(QtAction::Enabled, sceneDataFieldDescr, [](const DAVA::Any& value) -> DAVA::Any {
                return value.CanCast<SceneData::TSceneType>() && value.Cast<SceneData::TSceneType>().Get() != nullptr;
            });
        }
        action->setShortcuts({ QKeySequence(Qt::CTRL + Qt::Key_P) });
        action->setShortcutContext(Qt::WindowShortcut);
        connections.AddConnection(action, &QAction::triggered, DAVA::MakeFunction(this, &ObjectPlacementModule::OnPlaceAndAlign));
        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateMenuPoint(MenuItems::menuEdit,
                                                        { InsertionParams::eInsertionMethod::AfterItem, "Place on landscape" }));
        placementInfo.AddPlacementPoint(CreateToolbarPoint(toolBarName));

        ui->AddAction(mainWindowKey, placementInfo, action);
    }
}

void ObjectPlacementModule::OnPlaceOnLandscape()
{
    DAVA::DataContext* context = GetAccessor()->GetActiveContext();
    ObjectPlacementData* data = context->GetData<ObjectPlacementData>();
    data->objectPlacementSystem->PlaceOnLandscape();
}

void ObjectPlacementModule::OnSnapToLandscape()
{
    DAVA::DataContext* context = GetAccessor()->GetActiveContext();
    ObjectPlacementData* data = context->GetData<ObjectPlacementData>();
    bool snapToLandscapeEnabled = data->GetSnapToLandscape();
    data->SetSnapToLandscape(!snapToLandscapeEnabled);
}

void ObjectPlacementModule::OnPlaceAndAlign()
{
    DAVA::DataContext* context = GetAccessor()->GetActiveContext();
    ObjectPlacementData* data = context->GetData<ObjectPlacementData>();
    data->objectPlacementSystem->PlaceAndAlign();
}

DAVA_VIRTUAL_REFLECTION_IMPL(ObjectPlacementModule)
{
    DAVA::ReflectionRegistrator<ObjectPlacementModule>::Begin()
    .ConstructorByPointer()
    .End();
}

DECL_TARC_MODULE(ObjectPlacementModule);
