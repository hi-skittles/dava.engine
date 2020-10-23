#include "Classes/Selection/SelectionModule.h"
#include "Classes/Selection/SelectionData.h"

#include "Classes/Application/REGlobal.h"
#include "Classes/SceneManager/SceneData.h"

#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/Qt/Scene/System/ModifSystem.h"
#include "Classes/Qt/Scene/System/HoodSystem.h"
#include "Classes/Qt/Scene/System/WayEditSystem.h"

#include "TArc/WindowSubSystem/QtAction.h"
#include "TArc/WindowSubSystem/UI.h"
#include "TArc/WindowSubSystem/ActionUtils.h"
#include "TArc/Utils/ModuleCollection.h"

#include "Reflection/ReflectionRegistrator.h"
#include "Logger/Logger.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Systems/RenderUpdateSystem.h"

void SelectionModule::OnContextCreated(DAVA::TArc::DataContext* context)
{
    SceneData* sceneData = context->GetData<SceneData>();
    SceneEditor2* scene = sceneData->GetScene().Get();
    DVASSERT(scene != nullptr);

    std::unique_ptr<SelectionData> selectionData = std::make_unique<SelectionData>();
    selectionData->selectionSystem.reset(new SelectionSystem(scene));
    scene->AddSystem(selectionData->selectionSystem.get(), 0, DAVA::Scene::SCENE_SYSTEM_REQUIRE_PROCESS | DAVA::Scene::SCENE_SYSTEM_REQUIRE_INPUT, scene->renderUpdateSystem, scene->heightmapEditorSystem);

    //TODO: Workaround to save old process
    selectionData->selectionSystem->AddDelegate(scene->modifSystem);
    selectionData->selectionSystem->AddDelegate(scene->hoodSystem);
    selectionData->selectionSystem->AddDelegate(scene->wayEditSystem);
    selectionData->selectionSystem->EnableSystem();
    //END of TODO

    context->CreateData(std::move(selectionData));
}

void SelectionModule::OnContextDeleted(DAVA::TArc::DataContext* context)
{
    using namespace DAVA::TArc;

    SceneData* sceneData = context->GetData<SceneData>();
    SceneEditor2* scene = sceneData->GetScene().Get();

    SelectionData* selectionData = context->GetData<SelectionData>();
    scene->RemoveSystem(selectionData->selectionSystem.get());
}

void SelectionModule::PostInit()
{
    DAVA::TArc::UI* ui = GetUI();
    CreateModuleActions(ui);
}

void SelectionModule::CreateModuleActions(DAVA::TArc::UI* ui)
{
    using namespace DAVA::TArc;

    ContextAccessor* accessor = GetAccessor();

    // On Scene Selection
    {
        QtAction* action = new QtAction(accessor, QIcon(":/QtIcons/object_select.png"), QString("Lock Selection by Mouse"));

        { //enable/disable
            FieldDescriptor fieldDescr;
            fieldDescr.fieldName = DAVA::FastName(SceneData::scenePropertyName);
            fieldDescr.type = DAVA::ReflectedTypeDB::Get<SceneData>();
            action->SetStateUpdationFunction(QtAction::Enabled, fieldDescr, [](const DAVA::Any& value) -> DAVA::Any {
                return value.CanCast<SceneData::TSceneType>() && value.Cast<SceneData::TSceneType>().Get() != nullptr;
            });
        }

        { //text
            FieldDescriptor fieldDescr;
            fieldDescr.fieldName = DAVA::FastName(SelectionData::selectionAllowedPropertyName);
            fieldDescr.type = DAVA::ReflectedTypeDB::Get<SelectionData>();
            action->SetStateUpdationFunction(QtAction::Text, fieldDescr, [](const DAVA::Any& value) -> DAVA::Any {
                bool allowed = (value.CanGet<bool>() && value.Get<bool>());
                if (allowed)
                    return DAVA::String("Lock Selection by Mouse");
                return DAVA::String("Unlock Selection by Mouse");
            });
        }

        connections.AddConnection(action, &QAction::triggered, DAVA::Bind(&SelectionModule::SelectionByMouseChanged, this));

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateMenuPoint(MenuItems::menuEdit, { InsertionParams::eInsertionMethod::AfterItem, "actionModifySnapToLandscape" }));
        placementInfo.AddPlacementPoint(CreateStatusbarPoint(true, 0, { InsertionParams::eInsertionMethod::AfterItem, "actionShowStaticOcclusion" }));

        ui->AddAction(DAVA::TArc::mainWindowKey, placementInfo, action);
    }
}

void SelectionModule::SelectionByMouseChanged()
{
    DAVA::TArc::DataContext* context = GetAccessor()->GetActiveContext();
    SelectionData* selectionData = context->GetData<SelectionData>();
    bool allowed = selectionData->IsSelectionAllowed();
    selectionData->SetSelectionAllowed(!allowed);
}

DAVA_VIRTUAL_REFLECTION_IMPL(SelectionModule)
{
    DAVA::ReflectionRegistrator<SelectionModule>::Begin()
    .ConstructorByPointer()
    .End();
}

DECL_GUI_MODULE(SelectionModule);
