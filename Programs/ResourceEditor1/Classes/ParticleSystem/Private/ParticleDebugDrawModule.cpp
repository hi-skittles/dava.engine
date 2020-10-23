#include "Classes/ParticleSystem/ParticleDebugDrawModule.h"
#include "Classes/Application/RESettings.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/Selection/SelectionData.h"
#include "Classes/Selection/SelectableGroup.h"

#include <TArc/Utils/ModuleCollection.h>
#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/Controls/CheckBox.h>
#include <TArc/Controls/ComboBox.h>
#include <TArc/Controls/QtBoxLayouts.h>
#include <TArc/Core/ContextAccessor.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/DataProcessing/DataNode.h>

#include <Base/BaseTypes.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Systems/ParticleEffectDebugDrawSystem.h>
#include <Render/Highlevel/RenderObject.h>

#include <QWidget>
#include <QAction>

using DAVA::eParticleDebugDrawMode;

ENUM_DECLARE(eParticleDebugDrawMode)
{
    ENUM_ADD_DESCR(eParticleDebugDrawMode::LOW_ALPHA, "Low alpha");
    ENUM_ADD_DESCR(eParticleDebugDrawMode::WIREFRAME, "Wireframe");
    ENUM_ADD_DESCR(eParticleDebugDrawMode::OVERDRAW, "Overdraw");
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

class ParticleDebugDrawData : public DAVA::TArc::DataNode
{
public:
    bool isSystemOn = false;
    bool drawOnlySelected = false;
    eParticleDebugDrawMode drawMode = eParticleDebugDrawMode::OVERDRAW;
    DAVA::UnorderedSet<DAVA::RenderObject*> selectedParticles;
    DAVA::float32 alphaThreshold = 0.05f;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ParticleDebugDrawData, DAVA::TArc::DataNode)
    {
    }
};

void ParticleDebugDrawModule::OnContextCreated(DAVA::TArc::DataContext* context)
{
}

void ParticleDebugDrawModule::OnContextDeleted(DAVA::TArc::DataContext* context)
{
}

void ParticleDebugDrawModule::PostInit()
{
    using namespace DAVA::TArc;

    ContextAccessor* accessor = GetAccessor();
    accessor->GetGlobalContext()->CreateData(std::make_unique<ParticleDebugDrawData>());

    QWidget* w = new QWidget();
    QtHBoxLayout* layout = new QtHBoxLayout(w);
    layout->setMargin(0);
    layout->setSpacing(4);

    UI* ui = GetUI();

    {
        CheckBox::Params params(accessor, ui, DAVA::TArc::mainWindowKey);
        params.fields[CheckBox::Fields::IsReadOnly] = "readOnly";
        params.fields[CheckBox::Fields::Checked] = "isEnabledProperty";
        layout->AddControl(new CheckBox(params, accessor, DAVA::Reflection::Create(DAVA::ReflectedObject(this)), w));
    }

    {
        ComboBox::Params params(accessor, ui, DAVA::TArc::mainWindowKey);
        params.fields[ComboBox::Fields::IsReadOnly] = "readOnly";
        params.fields[ComboBox::Fields::Value] = "drawModeProperty";
        layout->AddControl(new ComboBox(params, accessor, DAVA::Reflection::Create(DAVA::ReflectedObject(this)), w));
    }

    {
        CheckBox::Params params(accessor, ui, DAVA::TArc::mainWindowKey);
        params.fields[CheckBox::Fields::IsReadOnly] = "readOnly";
        params.fields[CheckBox::Fields::Checked] = "drawSelectedProperty";
        layout->AddControl(new CheckBox(params, accessor, DAVA::Reflection::Create(DAVA::ReflectedObject(this)), w));
    }

    QString toolbarName = "ParticleSystemToolbar";
    ActionPlacementInfo toolbarTogglePlacement(CreateMenuPoint(QList<QString>() << "View"
                                                                                << "Toolbars"));
    ui->DeclareToolbar(DAVA::TArc::mainWindowKey, toolbarTogglePlacement, toolbarName);

    QAction* action = new QAction(nullptr);
    AttachWidgetToAction(action, w);

    ActionPlacementInfo placementInfo(CreateToolbarPoint(toolbarName));
    ui->AddAction(DAVA::TArc::mainWindowKey, placementInfo, action);

    filedBinder.reset(new FieldBinder(accessor));
    DAVA::TArc::FieldDescriptor descr;
    descr.type = DAVA::ReflectedTypeDB::Get<SelectionData>();
    descr.fieldName = DAVA::FastName(SelectionData::selectionPropertyName);
    filedBinder->BindField(descr, DAVA::MakeFunction(this, &ParticleDebugDrawModule::OnSelectionChanged));
}

void ParticleDebugDrawModule::OnSelectionChanged(const DAVA::Any selection)
{
    SelectableGroup group = selection.Cast<SelectableGroup>(SelectableGroup());
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

eParticleDebugDrawMode ParticleDebugDrawModule::GetDrawMode() const
{
    return GetAccessor()->GetGlobalContext()->GetData<ParticleDebugDrawData>()->drawMode;
}

void ParticleDebugDrawModule::SetDrawMode(eParticleDebugDrawMode mode)
{
    GetAccessor()->GetGlobalContext()->GetData<ParticleDebugDrawData>()->drawMode = mode;
    UpdateSceneSystem();
}

void ParticleDebugDrawModule::UpdateSceneSystem()
{
    using namespace DAVA::TArc;
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

DAVA::UnorderedSet<DAVA::RenderObject*> ParticleDebugDrawModule::ProcessSelection(const SelectableGroup& group)
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
    .Field("drawModeProperty", &ParticleDebugDrawModule::GetDrawMode, &ParticleDebugDrawModule::SetDrawMode)[DAVA::M::EnumT<eParticleDebugDrawMode>()]
    .Field("drawSelectedProperty", &ParticleDebugDrawModule::GetDrawOnlySelected, &ParticleDebugDrawModule::SetDrawOnlySelected)
    [DAVA::M::ValueDescription(&ParticleDebugDrawModuleDetail::ParticlesDebugStytemDrawSelectedState)]
    .Field("readOnly", &ParticleDebugDrawModule::IsDisabled, nullptr)
    .End();
}

DECL_GUI_MODULE(ParticleDebugDrawModule);
