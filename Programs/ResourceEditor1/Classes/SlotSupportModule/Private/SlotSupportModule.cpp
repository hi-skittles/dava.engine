#include "Classes/SlotSupportModule/SlotSupportModule.h"
#include "Classes/SlotSupportModule/Private/EditorSlotSystem.h"
#include "Classes/SlotSupportModule/Private/EntityForSlotLoader.h"
#include "Classes/SlotSupportModule/Private/SlotComponentExtensions.h"
#include "Classes/SlotSupportModule/Private/SlotTemplatesData.h"
#include "Classes/SlotSupportModule/SlotSystemSettings.h"

#include "Classes/Interfaces/PropertyPanelInterface.h"
#include "Classes/SlotSupportModule/Private/EntityForSlotLoader.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/Project/ProjectManagerData.h"

#include "Classes/Qt/Scene/SceneEditor2.h"

#include <TArc/Utils/ModuleCollection.h>
#include <TArc/WindowSubSystem/QtAction.h>
#include <TArc/WindowSubSystem/ActionUtils.h>

#include <Entity/Component.h>
#include <Entity/ComponentUtils.h>
#include <Reflection/ReflectionRegistrator.h>

namespace SlotSupportModuleDetails
{
class SlotSupportData : public DAVA::TArc::DataNode
{
public:
    std::unique_ptr<EditorSlotSystem> system = nullptr;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SlotSupportData, DAVA::TArc::DataNode)
    {
        DAVA::ReflectionRegistrator<SlotSupportData>::Begin()
        .End();
    }
};

class SlotPropertyPanelExtensions : public DAVA::TArc::DataNode
{
public:
    std::shared_ptr<DAVA::TArc::ChildCreatorExtension> childCreator;
    std::shared_ptr<DAVA::TArc::EditorComponentExtension> editorCreator;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SlotPropertyPanelExtensions, DAVA::TArc::DataNode)
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
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(SlotSystemSettings);
}

void SlotSupportModule::OnContextCreated(DAVA::TArc::DataContext* context)
{
    using namespace DAVA;

    SceneData* data = context->GetData<SceneData>();
    RefPtr<SceneEditor2> scene = data->GetScene();

    SlotSupportModuleDetails::SlotSupportData* slotSupportData = new SlotSupportModuleDetails::SlotSupportData();
    context->CreateData(std::unique_ptr<DAVA::TArc::DataNode>(slotSupportData));
    slotSupportData->system.reset(new EditorSlotSystem(scene.Get(), GetAccessor()));

    scene->AddSystem(slotSupportData->system.get(), ComponentUtils::MakeMask<SlotComponent>(), Scene::SCENE_SYSTEM_REQUIRE_PROCESS, scene->slotSystem);
    scene->slotSystem->SetExternalEntityLoader(std::shared_ptr<SlotSystem::ExternalEntityLoader>(new EntityForSlotLoader(GetAccessor())));
}

void SlotSupportModule::OnContextDeleted(DAVA::TArc::DataContext* context)
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
    if (interfaceType == DAVA::Type::Instance<Interfaces::PropertyPanelInterface>())
    {
        Interfaces::PropertyPanelInterface* propertyPanel = QueryInterface<Interfaces::PropertyPanelInterface>();
        using namespace SlotSupportModuleDetails;
        SlotPropertyPanelExtensions* data = GetAccessor()->GetGlobalContext()->GetData<SlotPropertyPanelExtensions>();
        propertyPanel->RegisterExtension(data->childCreator);
        propertyPanel->RegisterExtension(data->editorCreator);
    }
}

void SlotSupportModule::OnBeforeInterfaceUnregistered(const DAVA::Type* interfaceType)
{
    if (interfaceType == DAVA::Type::Instance<Interfaces::PropertyPanelInterface>())
    {
        Interfaces::PropertyPanelInterface* propertyPanel = QueryInterface<Interfaces::PropertyPanelInterface>();
        using namespace SlotSupportModuleDetails;
        SlotPropertyPanelExtensions* data = GetAccessor()->GetGlobalContext()->GetData<SlotPropertyPanelExtensions>();
        propertyPanel->UnregisterExtension(data->childCreator);
        propertyPanel->UnregisterExtension(data->editorCreator);
    }
}

void SlotSupportModule::ReloadConfig() const
{
    ProjectManagerData* data = GetAccessor()->GetGlobalContext()->GetData<ProjectManagerData>();
    ParseConfig(data->GetProjectPath());
}

void SlotSupportModule::ParseConfig(const DAVA::Any& v) const
{
    SlotTemplatesData* slotTemplates = GetAccessor()->GetGlobalContext()->GetData<SlotTemplatesData>();
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
    GetAccessor()->GetGlobalContext()->CreateData(std::unique_ptr<DAVA::TArc::DataNode>(extData));

    SlotTemplatesData* slotTemplates = new SlotTemplatesData();
    GetAccessor()->GetGlobalContext()->CreateData(std::unique_ptr<DAVA::TArc::DataNode>(slotTemplates));

    DAVA::TArc::ContextAccessor* accessor = GetAccessor();
    fieldBinder.reset(new DAVA::TArc::FieldBinder(accessor));

    DAVA::TArc::FieldDescriptor descr;
    descr.fieldName = DAVA::FastName(ProjectManagerData::ProjectPathProperty);
    descr.type = DAVA::ReflectedTypeDB::Get<ProjectManagerData>();
    fieldBinder->BindField(descr, DAVA::MakeFunction(this, &SlotSupportModule::ParseConfig));

    DAVA::TArc::QtAction* action = new DAVA::TArc::QtAction(accessor, "Reload slot templates");
    action->SetStateUpdationFunction(DAVA::TArc::QtAction::Enabled, descr, [](const DAVA::Any& v) {
        return !v.IsEmpty();
    });

    connections.AddConnection(action, &QAction::triggered, DAVA::MakeFunction(this, &SlotSupportModule::ReloadConfig));

    DAVA::TArc::ActionPlacementInfo placement(DAVA::TArc::CreateMenuPoint(QList<QString>() << "DebugFunctions"));
    GetUI()->AddAction(DAVA::TArc::mainWindowKey, placement, action);
}

DECL_GUI_MODULE(SlotSupportModule);
