#include "Classes/ParticleSystem/ParticleDebugDrawModule.h"

#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/DataNodes/SelectableGroup.h>

#include <TArc/Controls/CheckBox.h>
#include <TArc/Controls/ComboBox.h>
#include <TArc/Controls/QtBoxLayouts.h>
#include <TArc/Core/ContextAccessor.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/DataProcessing/TArcDataNode.h>
#include <TArc/Utils/ModuleCollection.h>
#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/WindowSubSystem/UI.h>

#include <Base/BaseTypes.h>
#include <Render/Highlevel/RenderObject.h>
#include <Scene3D/Components/ParticleEffectComponent.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Systems/ParticleEffectDebugDrawSystem.h>

#include <QWidget>
#include <QAction>

ENUM_DECLARE(DAVA::eParticleDebugDrawMode)
{
    ENUM_ADD_DESCR(DAVA::eParticleDebugDrawMode::LOW_ALPHA, "Low alpha");
    ENUM_ADD_DESCR(DAVA::eParticleDebugDrawMode::WIREFRAME, "Wireframe");
    ENUM_ADD_DESCR(DAVA::eParticleDebugDrawMode::OVERDRAW, "Overdraw");
}

namespace ParticleDebugDrawModuleDetail
{
DAVA::String ParticlesDebugStytemState(const DAVA::Any& isSystemOn)
{
    return "Particles debug";
}

DAVA::String ParticlesDebugStytemDrawSelectedState(const DAVA::Any& drawOnlySelected)
{
    return "Show only selected";
}
}

class ParticleDebugDrawData : public DAVA::TArcDataNode
{
public:
    bool isSystemOn = false;
    bool drawOnlySelected = false;
    DAVA::eParticleDebugDrawMode drawMode = DAVA::eParticleDebugDrawMode::OVERDRAW;
    DAVA::UnorderedSet<DAVA::RenderObject*> selectedParticles;
    DAVA::float32 alphaThreshold = 0.05f;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ParticleDebugDrawData, DAVA::TArcDataNode)
    {
    }
};

void ParticleDebugDrawModule::OnContextCreated(DAVA::DataContext* context)
{
}

void ParticleDebugDrawModule::OnContextDeleted(DAVA::DataContext* context)
{
}

void ParticleDebugDrawModule::PostInit()
{
    using namespace DAVA;

    ContextAccessor* accessor = GetAccessor();
    accessor->GetGlobalContext()->CreateData(std::make_unique<ParticleDebugDrawData>());

    QWidget* w = new QWidget();
    QtHBoxLayout* layout = new QtHBoxLayout(w);
    layout->setMargin(0);
    layout->setSpacing(4);

    UI* ui = GetUI();

    {
        CheckBox::Params params(accessor, ui, DAVA::mainWindowKey);
        params.fields[CheckBox::Fields::IsReadOnly] = "readOnly";
        params.fields[CheckBox::Fields::Checked] = "isEnabledProperty";
        layout->AddControl(new CheckBox(params, accessor, DAVA::Reflection::Create(DAVA::ReflectedObject(this)), w));
    }

    {
        ComboBox::Params params(accessor, ui, DAVA::mainWindowKey);
        params.fields[ComboBox::Fields::IsReadOnly] = "readOnly";
        params.fields[ComboBox::Fields::Value] = "drawModeProperty";
        layout->AddControl(new ComboBox(params, accessor, DAVA::Reflection::Create(DAVA::ReflectedObject(this)), w));
    }

    {
        CheckBox::Params params(accessor, ui, DAVA::mainWindowKey);
        params.fields[CheckBox::Fields::IsReadOnly] = "readOnly";
        params.fields[CheckBox::Fields::Checked] = "drawSelectedProperty";
        layout->AddControl(new CheckBox(params, accessor, DAVA::Reflection::Create(DAVA::ReflectedObject(this)), w));
    }

    QString toolbarName = "ParticleSystemToolbar";
    ActionPlacementInfo toolbarTogglePlacement(CreateMenuPoint(QList<QString>() << "View"
                                                                                << "Toolbars"));
    ui->DeclareToolbar(DAVA::mainWindowKey, toolbarTogglePlacement, toolbarName);

    QAction* action = new QAction(nullptr);
    AttachWidgetToAction(action, w);

    ActionPlacementInfo placementInfo(CreateToolbarPoint(toolbarName));
    ui->AddAction(DAVA::mainWindowKey, placementInfo, action);

    filedBinder.reset(new FieldBinder(accessor));
    DAVA::FieldDescriptor descr;
    descr.type = DAVA::ReflectedTypeDB::Get<SelectionData>();
    descr.fieldName = DAVA::FastName(SelectionData::selectionPropertyName);
    filedBinder->BindField(descr, DAVA::MakeFunction(this, &ParticleDebugDrawModule::OnSelectionChanged));
}

void ParticleDebugDrawModule::OnSelectionChanged(const DAVA::Any selection)
{
    DAVA::SelectableGroup group = selection.Cast<DAVA::SelectableGroup>(DAVA::SelectableGroup());
    GetAccessor()->GetGlobalContext()->GetData<ParticleDebugDrawData>()->selectedParticles = ProcessSelection(group);
    UpdateSceneSystem();
}

bool ParticleDebugDrawModule::GetSystemEnabledState() const
{
    return GetAccessor()->GetGlobalContext()->GetData<ParticleDebugDrawData>()->isSystemOn;
}

void ParticleDebugDrawModule::SetSystemEnabledState(bool enabled)
{
    GetAccessor()->GetGlobalContext()->GetData<ParticleDebugDrawData>()->isSystemOn = enabled;
    DAVA::Renderer::GetOptions()->SetOption(DAVA::RenderOptions::DEBUG_DRAW_PARTICLES, enabled);
    UpdateSceneSystem();
}

bool ParticleDebugDrawModule::GetDrawOnlySelected() const
{
    return GetAccessor()->GetGlobalContext()->GetData<ParticleDebugDrawData>()->drawOnlySelected;
}

void ParticleDebugDrawModule::SetDrawOnlySelected(bool drawOnlySelected)
{
    GetAccessor()->GetGlobalContext()->GetData<ParticleDebugDrawData>()->drawOnlySelected = drawOnlySelected;
    UpdateSceneSystem();
}

bool ParticleDebugDrawModule::IsDisabled() const
{
    return GetAccessor()->GetContextCount() == 0;
}

DAVA::eParticleDebugDrawMode ParticleDebugDrawModule::GetDrawMode() const
{
    return GetAccessor()->GetGlobalContext()->GetData<ParticleDebugDrawData>()->drawMode;
}

void ParticleDebugDrawModule::SetDrawMode(DAVA::eParticleDebugDrawMode mode)
{
    GetAccessor()->GetGlobalContext()->GetData<ParticleDebugDrawData>()->drawMode = mode;
    UpdateSceneSystem();
}

void ParticleDebugDrawModule::UpdateSceneSystem()
{
    using namespace DAVA;
    ContextAccessor* accessor = GetAccessor();
    ParticleDebugDrawData* data = GetAccessor()->GetGlobalContext()->GetData<ParticleDebugDrawData>();

    GeneralSettings* settings = accessor->GetGlobalContext()->GetData<GeneralSettings>();

    accessor->ForEachContext([data, settings](DataContext& ctx)
                             {
                                 SceneData::TSceneType scene = ctx.GetData<SceneData>()->GetScene();
                                 DAVA::ParticleEffectDebugDrawSystem* particleEffectDebugDrawSystem = scene->GetParticleEffectDebugDrawSystem();
                                 if (particleEffectDebugDrawSystem != nullptr)
                                 {
                                     particleEffectDebugDrawSystem->SetDrawMode(data->drawMode);
                                     particleEffectDebugDrawSystem->SetIsDrawOnlySelected(data->drawOnlySelected);
                                     particleEffectDebugDrawSystem->SetSelectedParticles(data->selectedParticles);
                                     particleEffectDebugDrawSystem->SetAlphaThreshold(settings->particleDebugAlphaTheshold);
                                 }
                             });
}

DAVA::UnorderedSet<DAVA::RenderObject*> ParticleDebugDrawModule::ProcessSelection(const DAVA::SelectableGroup& group)
{
    DAVA::uint32 count = static_cast<DAVA::uint32>(group.GetSize());
    DAVA::UnorderedSet<DAVA::RenderObject*> particleObjects;
    for (auto entity : group.ObjectsOfType<DAVA::Entity>())
    {
        DAVA::ParticleEffectComponent* particleComponent = entity->GetComponent<DAVA::ParticleEffectComponent>();
        if (particleComponent != nullptr)
            particleObjects.insert(particleComponent->GetRenderObject());
    }
    return particleObjects;
}

DAVA_VIRTUAL_REFLECTION_IMPL(ParticleDebugDrawModule)
{
    DAVA::ReflectionRegistrator<ParticleDebugDrawModule>::Begin()
    .ConstructorByPointer()
    .Field("isEnabledProperty", &ParticleDebugDrawModule::GetSystemEnabledState, &ParticleDebugDrawModule::SetSystemEnabledState)
    [DAVA::M::ValueDescription(&ParticleDebugDrawModuleDetail::ParticlesDebugStytemState)]
    .Field("drawModeProperty", &ParticleDebugDrawModule::GetDrawMode, &ParticleDebugDrawModule::SetDrawMode)[DAVA::M::EnumT<DAVA::eParticleDebugDrawMode>()]
    .Field("drawSelectedProperty", &ParticleDebugDrawModule::GetDrawOnlySelected, &ParticleDebugDrawModule::SetDrawOnlySelected)
    [DAVA::M::ValueDescription(&ParticleDebugDrawModuleDetail::ParticlesDebugStytemDrawSelectedState)]
    .Field("readOnly", &ParticleDebugDrawModule::IsDisabled, nullptr)
    .End();
}

DECL_TARC_MODULE(ParticleDebugDrawModule);
