#include "Classes/SlotSupportModule/SlotSupportModule.h"
#include "Classes/SlotSupportModule/Private/EntityForSlotLoader.h"
#include "Classes/SlotSupportModule/Private/SlotComponentExtensions.h"

#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/SlotTemplatesData.h>
#include <REPlatform/DataNodes/Settings/SlotSystemSettings.h>
#include <REPlatform/Global/PropertyPanelInterface.h>
#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/Systems/EditorSlotSystem.h>

#include <TArc/Utils/ModuleCollection.h>
#include <TArc/WindowSubSystem/QtAction.h>
#include <TArc/WindowSubSystem/ActionUtils.h>

#include <Entity/Component.h>
#include <Entity/ComponentUtils.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Entity/Component.h>

namespace SlotSupportModuleDetails
{
class SlotSupportData : public DAVA::TArcDataNode
{
public:
    std::unique_ptr<DAVA::EditorSlotSystem> system = nullptr;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SlotSupportData, DAVA::TArcDataNode)
    {
        DAVA::ReflectionRegistrator<SlotSupportData>::Begin()
        .End();
    }
};

class SlotPropertyPanelExtensions : public DAVA::TArcDataNode
{
public:
    std::shared_ptr<DAVA::ChildCreatorExtension> childCreator;
    std::shared_ptr<DAVA::EditorComponentExtension> editorCreator;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SlotPropertyPanelExtensions, DAVA::TArcDataNode)
    {
        DAVA::ReflectionRegistrator<SlotPropertyPanelExtensions>::Begin()
        .End();
    }
};
}

DAVA_VIRTUAL_REFLECTION_IMPL(SlotSupportModule)
{
    DAVA::ReflectionRegistrator<SlotSupportModule>::Begin()
    .ConstructorByPointer()
    .End();
}

SlotSupportModule::SlotSupportModule()
{
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(DAVA::SlotSystemSettings);
    DAVA_REFLECTION_REGISTER_CUSTOM_PERMANENT_NAME(DAVA::LoadedSlotItemComponent, "LoadedSlotItemComponent");
}

void SlotSupportModule::OnContextCreated(DAVA::DataContext* context)
{
    using namespace DAVA;

    SceneData* data = context->GetData<SceneData>();
    RefPtr<SceneEditor2> scene = data->GetScene();

    SlotSupportModuleDetails::SlotSupportData* slotSupportData = new SlotSupportModuleDetails::SlotSupportData();
    context->CreateData(std::unique_ptr<DAVA::TArcDataNode>(slotSupportData));
    slotSupportData->system.reset(new EditorSlotSystem(scene.Get(), GetAccessor()));

    scene->AddSystem(slotSupportData->system.get(), ComponentUtils::MakeMask<SlotComponent>(), Scene::SCENE_SYSTEM_REQUIRE_PROCESS, scene->slotSystem);
    scene->slotSystem->SetExternalEntityLoader(std::shared_ptr<SlotSystem::ExternalEntityLoader>(new EntityForSlotLoader(GetAccessor())));
}

void SlotSupportModule::OnContextDeleted(DAVA::DataContext* context)
{
    using namespace DAVA;

    {
        SceneData* data = context->GetData<SceneData>();
        RefPtr<SceneEditor2> scene = data->GetScene();

        SlotSupportModuleDetails::SlotSupportData* slotSupportData = context->GetData<SlotSupportModuleDetails::SlotSupportData>();
        scene->RemoveSystem(slotSupportData->system.get());
    }

    context->DeleteData<SlotSupportModuleDetails::SlotSupportData>();
}

void SlotSupportModule::OnInterfaceRegistered(const DAVA::Type* interfaceType)
{
    if (interfaceType == DAVA::Type::Instance<DAVA::PropertyPanelInterface>())
    {
        DAVA::PropertyPanelInterface* propertyPanel = QueryInterface<DAVA::PropertyPanelInterface>();
        using namespace SlotSupportModuleDetails;
        SlotPropertyPanelExtensions* data = GetAccessor()->GetGlobalContext()->GetData<SlotPropertyPanelExtensions>();
        propertyPanel->RegisterExtension(data->childCreator);
        propertyPanel->RegisterExtension(data->editorCreator);
    }
}

void SlotSupportModule::OnBeforeInterfaceUnregistered(const DAVA::Type* interfaceType)
{
    if (interfaceType == DAVA::Type::Instance<DAVA::PropertyPanelInterface>())
    {
        DAVA::PropertyPanelInterface* propertyPanel = QueryInterface<DAVA::PropertyPanelInterface>();
        using namespace SlotSupportModuleDetails;
        SlotPropertyPanelExtensions* data = GetAccessor()->GetGlobalContext()->GetData<SlotPropertyPanelExtensions>();
        propertyPanel->UnregisterExtension(data->childCreator);
        propertyPanel->UnregisterExtension(data->editorCreator);
    }
}

void SlotSupportModule::ReloadConfig() const
{
    DAVA::ProjectManagerData* data = GetAccessor()->GetGlobalContext()->GetData<DAVA::ProjectManagerData>();
    ParseConfig(data->GetProjectPath());
}

void SlotSupportModule::ParseConfig(const DAVA::Any& v) const
{
    DAVA::SlotTemplatesData* slotTemplates = GetAccessor()->GetGlobalContext()->GetData<DAVA::SlotTemplatesData>();
    slotTemplates->Clear();
    if (v.IsEmpty() == false)
    {
        DAVA::FilePath projectPath = v.Cast<DAVA::FilePath>();
        slotTemplates->ParseConfig(projectPath + "/SlotTemplates.yaml");
    }
}

void SlotSupportModule::PostInit()
{
    using namespace SlotSupportModuleDetails;
    SlotPropertyPanelExtensions* extData = new SlotPropertyPanelExtensions();
    extData->childCreator.reset(new PropertyPanel::SlotComponentChildCreator());
    extData->editorCreator.reset(new PropertyPanel::SlotComponentEditorCreator());
    GetAccessor()->GetGlobalContext()->CreateData(std::unique_ptr<DAVA::TArcDataNode>(extData));

    DAVA::SlotTemplatesData* slotTemplates = new DAVA::SlotTemplatesData();
    GetAccessor()->GetGlobalContext()->CreateData(std::unique_ptr<DAVA::TArcDataNode>(slotTemplates));

    DAVA::ContextAccessor* accessor = GetAccessor();
    fieldBinder.reset(new DAVA::FieldBinder(accessor));

    DAVA::FieldDescriptor descr;
    descr.fieldName = DAVA::FastName(DAVA::ProjectManagerData::ProjectPathProperty);
    descr.type = DAVA::ReflectedTypeDB::Get<DAVA::ProjectManagerData>();
    fieldBinder->BindField(descr, DAVA::MakeFunction(this, &SlotSupportModule::ParseConfig));

    DAVA::QtAction* action = new DAVA::QtAction(accessor, "Reload slot templates");
    action->SetStateUpdationFunction(DAVA::QtAction::Enabled, descr, [](const DAVA::Any& v) {
        return !v.IsEmpty();
    });

    connections.AddConnection(action, &QAction::triggered, DAVA::MakeFunction(this, &SlotSupportModule::ReloadConfig));

    DAVA::ActionPlacementInfo placement(DAVA::CreateMenuPoint(QList<QString>() << "DebugFunctions"));
    GetUI()->AddAction(DAVA::mainWindowKey, placement, action);

    GetAccessor()->GetEngineContext()->componentManager->RegisterComponent<DAVA::LoadedSlotItemComponent>();
}

DECL_TARC_MODULE(SlotSupportModule);
