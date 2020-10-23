#include "Classes/TextModule/TextModule.h"
#include "Classes/TextModule/Private/EditorTextSystem.h"
#include "Classes/TextModule/Private/TextModuleData.h"

#include <REPlatform/Global/SceneTree/CreateEntitySupport.h>

#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/Scene/SceneEditor2.h>

#include <TArc/DataProcessing/Common.h>

#include <TArc/Utils/ModuleCollection.h>
#include <TArc/Utils/Utils.h>
#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/WindowSubSystem/QtAction.h>
#include <TArc/WindowSubSystem/UI.h>

#include <Entity/ComponentUtils.h>
#include <Engine/PlatformApiQt.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Scene3D/Components/TextComponent.h>

namespace TextModuleDetail
{
class TextEntityCreator : public DAVA::SimpleEntityCreator
{
    using TBase = DAVA::SimpleEntityCreator;

public:
    static DAVA::RefPtr<DAVA::Entity> CreateEntity()
    {
        DAVA::RefPtr<DAVA::Entity> textEntity(new DAVA::Entity());
        textEntity->AddComponent(new DAVA::TextComponent());
        textEntity->SetName("TextEntity");

        return textEntity;
    }

    TextEntityCreator()
        : TBase(eMenuPointOrder::TEXT_ENTITY, DAVA::SharedIcon(":/QtIcons/text_component.png"),
                QStringLiteral("Text Entity"), &TextEntityCreator::CreateEntity)
    {
    }

    DAVA_VIRTUAL_REFLECTION(TextEntityCreator, TBase);
};

DAVA_VIRTUAL_REFLECTION_IMPL(TextEntityCreator)
{
    DAVA::ReflectionRegistrator<TextEntityCreator>::Begin()
    .ConstructorByPointer()
    .End();
}

} // namespace TextModuleDetail

TextModule::TextModule()
{
    using namespace TextModuleDetail;
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(TextEntityCreator);
}

void TextModule::PostInit()
{
    using namespace DAVA;

    QtAction* action = new QtAction(GetAccessor(), QIcon(":/QtIcons/text_component.png"), QString("Text Drawing Enabled"));
    { // checked-unchecked and text
        FieldDescriptor fieldDescr;
        fieldDescr.fieldName = DAVA::FastName(TextModuleData::drawingEnabledPropertyName);
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<TextModuleData>();
        action->SetStateUpdationFunction(QtAction::Checked, fieldDescr, [](const DAVA::Any& value) -> DAVA::Any {
            return value.Get<bool>(false);
        });
        action->SetStateUpdationFunction(QtAction::Text, fieldDescr, [](const DAVA::Any& value) -> DAVA::Any {
            if (value.Get<bool>(false))
                return DAVA::String("Text Drawing Enabled");
            return DAVA::String("Text Drawing Disabled");
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

    KeyBindableActionInfo info;
    info.blockName = "Editor";
    info.context = Qt::WidgetWithChildrenShortcut;
    info.defaultShortcuts.push_back(QKeySequence("Ctrl+F"));
    MakeActionKeyBindable(action, info);

    PlatformApi::Qt::GetRenderWidget()->addAction(action);

    connections.AddConnection(action, &QAction::triggered, DAVA::Bind(&TextModule::ChangeDrawingState, this));

    ActionPlacementInfo placementInfo;
    placementInfo.AddPlacementPoint(CreateStatusbarPoint(true, 0, { InsertionParams::eInsertionMethod::AfterItem, "actionShowStaticOcclusion" }));

    GetUI()->AddAction(DAVA::mainWindowKey, placementInfo, action);
}

void TextModule::OnContextCreated(DAVA::DataContext* context)
{
    using namespace DAVA;
    SceneData* sceneData = context->GetData<SceneData>();
    SceneEditor2* scene = sceneData->GetScene().Get();
    DVASSERT(scene != nullptr);

    std::unique_ptr<TextModuleData> moduleData = std::make_unique<TextModuleData>();
    moduleData->editorTextSystem.reset(new EditorTextSystem(scene));
    moduleData->editorTextSystem->EnableSystem();
    scene->AddSystem(moduleData->editorTextSystem.get(), DAVA::ComponentUtils::MakeMask<DAVA::TextComponent>());

    context->CreateData(std::move(moduleData));
}

void TextModule::OnContextDeleted(DAVA::DataContext* context)
{
    using namespace DAVA;
    using namespace DAVA;

    SceneData* sceneData = context->GetData<SceneData>();
    SceneEditor2* scene = sceneData->GetScene().Get();

    TextModuleData* moduleData = context->GetData<TextModuleData>();
    scene->RemoveSystem(moduleData->editorTextSystem.get());
}

void TextModule::ChangeDrawingState()
{
    DAVA::DataContext* context = GetAccessor()->GetActiveContext();
    TextModuleData* moduleData = context->GetData<TextModuleData>();

    bool enabled = moduleData->IsDrawingEnabled();
    moduleData->SetDrawingEnabled(!enabled);
}

DAVA_VIRTUAL_REFLECTION_IMPL(TextModule)
{
    DAVA::ReflectionRegistrator<TextModule>::Begin()
    .ConstructorByPointer()
    .End();
}

DECL_TARC_MODULE(TextModule);
