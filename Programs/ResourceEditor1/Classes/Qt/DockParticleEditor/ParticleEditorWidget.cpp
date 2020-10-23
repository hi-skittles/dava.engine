#include "ParticleEditorWidget.h"
#include "EmitterLayerWidget.h"
#include "LayerForceSimplifiedWidget.h"

#include "Classes/Application/REGlobal.h"
#include "Classes/Selection/SelectionData.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/Qt/DockParticleEditor/LayerForceWidget.h"

#include "Scene/System/EditorParticlesSystem.h"

#include "TArc/Core/FieldBinder.h"

#include "ui_mainwindow.h"
#include <QScrollBar>

ParticleEditorWidget::ParticleEditorWidget(QWidget* parent /* = 0*/)
    : QScrollArea(parent)
{
    setWidgetResizable(true);

    emitterLayerWidget = nullptr;
    layerForceSimplifiedWidget = nullptr;
    emitterPropertiesWidget = nullptr;
    effectPropertiesWidget = nullptr;
    layerForceWidget = nullptr;

    CreateInnerWidgets();

    auto dispatcher = SceneSignals::Instance();
    connect(dispatcher, &SceneSignals::ParticleLayerValueChanged, this, &ParticleEditorWidget::OnParticleLayerValueChanged);
    connect(dispatcher, &SceneSignals::ParticleEmitterLoaded, this, &ParticleEditorWidget::OnParticleEmitterLoaded);
    connect(dispatcher, &SceneSignals::ParticleEmitterSaved, this, &ParticleEditorWidget::OnParticleEmitterSaved);

    selectionFieldBinder.reset(new DAVA::TArc::FieldBinder(REGlobal::GetAccessor()));
    {
        DAVA::TArc::FieldDescriptor fieldDescr;
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<SelectionData>();
        fieldDescr.fieldName = DAVA::FastName(SelectionData::selectionPropertyName);
        selectionFieldBinder->BindField(fieldDescr, DAVA::MakeFunction(this, &ParticleEditorWidget::OnSelectionChanged));
    }
}

ParticleEditorWidget::~ParticleEditorWidget()
{
    DeleteInnerWidgets();
}

void ParticleEditorWidget::CreateInnerWidgets()
{
    effectPropertiesWidget = new ParticleEffectPropertiesWidget(this);
    effectPropertiesWidget->hide();

    emitterPropertiesWidget = new ParticleEmitterPropertiesWidget(this);
    emitterPropertiesWidget->hide();

    emitterLayerWidget = new EmitterLayerWidget(this);
    emitterLayerWidget->hide();

    layerForceSimplifiedWidget = new LayerForceSimplifiedWidget(this);
    layerForceSimplifiedWidget->hide();

    layerForceWidget = new LayerForceWidget(this);
    layerForceWidget->hide();

    widgetMode = MODE_NONE;
}

void ParticleEditorWidget::DeleteInnerWidgets()
{
    SAFE_DELETE(effectPropertiesWidget);
    SAFE_DELETE(emitterPropertiesWidget);
    SAFE_DELETE(emitterLayerWidget);
    SAFE_DELETE(layerForceSimplifiedWidget);
    SAFE_DELETE(layerForceWidget);
}

void ParticleEditorWidget::OnUpdate()
{
    switch (widgetMode)
    {
    case MODE_EMITTER:
    {
        emitterPropertiesWidget->Update();
        break;
    }

    case MODE_LAYER:
    {
        emitterLayerWidget->Update(false);
        break;
    }

    case MODE_SIMPLIFIED_FORCE:
    {
        layerForceSimplifiedWidget->Update();
        break;
    }
    case MODE_PARTICLE_FORCE:
    {
        layerForceWidget->Update();
        break;
    }

    default:
    {
        break;
    }
    }
}

void ParticleEditorWidget::OnValueChanged()
{
    // Update the particle editor widgets when the value on the emitter layer is changed.
    UpdateParticleEditorWidgets();
}

void ParticleEditorWidget::UpdateParticleEditorWidgets()
{
    if (MODE_EMITTER == widgetMode && emitterPropertiesWidget->GetEmitterInstance(emitterPropertiesWidget->GetActiveScene()))
    {
        UpdateVisibleTimelinesForParticleEmitter();
        return;
    }

    if (MODE_LAYER == widgetMode && emitterLayerWidget->GetLayer())
    {
        UpdateWidgetsForLayer();
        return;
    }
}

void ParticleEditorWidget::UpdateVisibleTimelinesForParticleEmitter()
{
    // Safety check.
    if (MODE_EMITTER != widgetMode || !emitterPropertiesWidget->GetEmitterInstance(emitterPropertiesWidget->GetActiveScene()))
    {
        return;
    }

    // Update the visibility of particular timelines based on the emitter type.
    bool radiusTimeLineVisible = false;
    bool angleTimeLineVisible = false;
    bool sizeTimeLineVisible = false;

    auto emitterInstance = emitterPropertiesWidget->GetEmitterInstance(emitterPropertiesWidget->GetActiveScene());
    switch (emitterInstance->GetEmitter()->emitterType)
    {
    case DAVA::ParticleEmitter::EMITTER_ONCIRCLE_VOLUME:
    case DAVA::ParticleEmitter::EMITTER_ONCIRCLE_EDGES:
    case DAVA::ParticleEmitter::EMITTER_SHOCKWAVE:
    {
        radiusTimeLineVisible = true;
        angleTimeLineVisible = true;
        break;
    }

    case DAVA::ParticleEmitter::EMITTER_RECT:
    {
        sizeTimeLineVisible = true;
    }

    default:
    {
        break;
    }
    }

    emitterPropertiesWidget->GetEmitterRadiusTimeline()->setVisible(radiusTimeLineVisible);
    emitterPropertiesWidget->GetEmitterAngleTimeline()->setVisible(angleTimeLineVisible);
    emitterPropertiesWidget->GetEmitterSizeTimeline()->setVisible(sizeTimeLineVisible);
}

void ParticleEditorWidget::UpdateWidgetsForLayer()
{
    if (MODE_LAYER != widgetMode || !emitterLayerWidget->GetLayer())
    {
        return;
    }
    EmitterLayerWidget::eLayerMode mode = EmitterLayerWidget::eLayerMode::REGULAR;
    if (emitterLayerWidget->GetLayer()->type == DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
        mode = EmitterLayerWidget::eLayerMode::SUPEREMITTER;
    else if (emitterLayerWidget->GetLayer()->type == DAVA::ParticleLayer::TYPE_PARTICLE_STRIPE)
        mode = EmitterLayerWidget::eLayerMode::STRIPE;

    emitterLayerWidget->SetLayerMode(mode);
}

void ParticleEditorWidget::HandleEmitterSelected(SceneEditor2* scene, DAVA::ParticleEffectComponent* effect, DAVA::ParticleEmitterInstance* emitter, bool forceUpdate)
{
    auto widgetInstance = emitterPropertiesWidget->GetEmitterInstance(scene);
    auto sameEmitter = (widgetInstance != nullptr) && (widgetInstance->GetEmitter() == emitter->GetEmitter());
    if ((emitter != nullptr) && (MODE_EMITTER == widgetMode) && (!forceUpdate && sameEmitter))
    {
        return;
    }

    SwitchEditorToEmitterMode(scene, effect, emitter);
}

void ParticleEditorWidget::OnSelectionChanged(const DAVA::Any& selectionAny)
{
    if (selectionAny.CanGet<SelectableGroup>())
    {
        const SelectableGroup& selection = selectionAny.Get<SelectableGroup>();

        SceneData* sceneData = REGlobal::GetActiveDataNode<SceneData>();
        SceneEditor2* scene = sceneData->GetScene().Get();

        ProcessSelection(scene, selection);
    }
    else
    {
        ResetEditorMode();
    }
}

void ParticleEditorWidget::ProcessSelection(SceneEditor2* scene, const SelectableGroup& selection)
{
    bool shouldReset = true;
    SCOPE_EXIT
    {
        if (shouldReset)
        {
            ResetEditorMode();
        }
    };

    if (selection.GetSize() != 1)
        return;

    const auto& obj = selection.GetFirst();
    if (obj.CanBeCastedTo<DAVA::Entity>())
    {
        DAVA::Entity* entity = obj.AsEntity();
        DAVA::ParticleEffectComponent* effect = entity->GetComponent<DAVA::ParticleEffectComponent>();
        if (effect != nullptr)
        {
            shouldReset = false;
            SwitchEditorToEffectMode(scene, effect);
        }
    }
    else if (obj.CanBeCastedTo<DAVA::ParticleEmitterInstance>())
    {
        shouldReset = false;
        DAVA::ParticleEmitterInstance* instance = obj.Cast<DAVA::ParticleEmitterInstance>();
        DAVA::ParticleEffectComponent* component = scene->particlesSystem->GetEmitterOwner(instance);
        SwitchEditorToEmitterMode(scene, component, instance);
    }
    else if (obj.CanBeCastedTo<DAVA::ParticleLayer>())
    {
        DAVA::ParticleLayer* layer = obj.Cast<DAVA::ParticleLayer>();
        EditorParticlesSystem* system = scene->particlesSystem;
        DAVA::ParticleEmitterInstance* instance = system->GetRootEmitterLayerOwner(layer);
        if (instance != nullptr)
        {
            shouldReset = false;
            DAVA::ParticleEffectComponent* component = system->GetEmitterOwner(instance);
            SwitchEditorToLayerMode(scene, component, instance, layer);
        }
    }
    else if (obj.CanBeCastedTo<DAVA::ParticleForceSimplified>())
    {
        DAVA::ParticleForceSimplified* force = obj.Cast<DAVA::ParticleForceSimplified>();
        DAVA::ParticleLayer* layer = scene->particlesSystem->GetForceOwner(force);
        if (layer != nullptr)
        {
            auto i = std::find(layer->GetSimplifiedParticleForces().begin(), layer->GetSimplifiedParticleForces().end(), force);
            if (i != layer->GetSimplifiedParticleForces().end())
            {
                shouldReset = false;
                SwitchEditorToForceSimplifiedMode(scene, layer, std::distance(layer->GetSimplifiedParticleForces().begin(), i));
            }
        }
    }
    else if (obj.CanBeCastedTo<DAVA::ParticleForce>())
    {
        DAVA::ParticleForce* force = obj.Cast<DAVA::ParticleForce>();
        DAVA::ParticleLayer* layer = scene->particlesSystem->GetForceOwner(force);
        if (layer != nullptr)
        {
            auto i = std::find(layer->GetParticleForces().begin(), layer->GetParticleForces().end(), force);
            if (i != layer->GetParticleForces().end())
            {
                shouldReset = false;
                SwitchEditorToForceMode(scene, layer, std::distance(layer->GetParticleForces().begin(), i));
            }
        }
    }
}

void ParticleEditorWidget::OnParticleLayerValueChanged(SceneEditor2* /*scene*/, DAVA::ParticleLayer* layer)
{
    if (MODE_LAYER != widgetMode || emitterLayerWidget->GetLayer() != layer)
    {
        return;
    }

    // Notify the Emitter Layer widget about its inner layer value is changed and
    // the widget needs to be resynchronized with its values.
    emitterLayerWidget->OnLayerValueChanged();
    emitterLayerWidget->Update(false);
}

void ParticleEditorWidget::OnParticleEmitterLoaded(SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter)
{
    // Handle in the same way emitter is selected to update the values. However
    // cause widget to be force updated.
    HandleEmitterSelected(scene, emitterPropertiesWidget->GetEffect(scene), emitter, true);
}

void ParticleEditorWidget::OnParticleEmitterSaved(SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter)
{
    // Handle in the same way emitter is selected to update the values. However
    // cause widget to be force updated.
    DAVA::ParticleEffectComponent* currEffect = emitterPropertiesWidget->GetEffect(scene);
    DAVA::ParticleEmitterInstance* currEmitter = emitterPropertiesWidget->GetEmitterInstance(scene);
    if (currEffect && (currEmitter->GetEmitter() == emitter->GetEmitter()))
    {
        HandleEmitterSelected(scene, currEffect, emitter, true);
    }
}

void ParticleEditorWidget::SwitchEditorToEffectMode(SceneEditor2* scene, DAVA::ParticleEffectComponent* effect)
{
    ResetEditorMode();

    if (!effect)
    {
        emit ChangeVisible(false);
        return;
    }

    emit ChangeVisible(true);

    effectPropertiesWidget->Init(scene, effect);
    setWidget(effectPropertiesWidget);
    effectPropertiesWidget->show();

    this->widgetMode = MODE_EFFECT;
}

void ParticleEditorWidget::SwitchEditorToEmitterMode(SceneEditor2* scene, DAVA::ParticleEffectComponent* effect, DAVA::ParticleEmitterInstance* emitter)
{
    ResetEditorMode();

    if (!emitter)
    {
        emit ChangeVisible(false);
        return;
    }

    emit ChangeVisible(true);
    this->widgetMode = MODE_EMITTER;

    emitterPropertiesWidget->Init(scene, effect, emitter, true);
    setWidget(emitterPropertiesWidget);
    emitterPropertiesWidget->show();

    connect(emitterPropertiesWidget,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnValueChanged()));

    UpdateParticleEditorWidgets();
}

void ParticleEditorWidget::SwitchEditorToLayerMode(SceneEditor2* scene, DAVA::ParticleEffectComponent* effect,
                                                   DAVA::ParticleEmitterInstance* emitter, DAVA::ParticleLayer* layer)
{
    ResetEditorMode();

    if (!emitter || !layer)
    {
        emit ChangeVisible(false);
        return;
    }

    emit ChangeVisible(true);
    this->widgetMode = MODE_LAYER;

    emitterLayerWidget->Init(scene, effect, emitter, layer, true);
    setWidget(emitterLayerWidget);
    emitterLayerWidget->show();

    connect(emitterLayerWidget,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnValueChanged()));

    UpdateParticleEditorWidgets();
}

void ParticleEditorWidget::SwitchEditorToForceSimplifiedMode(SceneEditor2* scene, DAVA::ParticleLayer* layer, DAVA::int32 forceIndex)
{
    ResetEditorMode();

    if (!layer)
    {
        emit ChangeVisible(false);
        return;
    }

    emit ChangeVisible(true);
    widgetMode = MODE_SIMPLIFIED_FORCE;

    layerForceSimplifiedWidget->Init(scene, layer, forceIndex, true);
    setWidget(layerForceSimplifiedWidget);
    layerForceSimplifiedWidget->show();
    connect(layerForceSimplifiedWidget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));

    UpdateParticleEditorWidgets();
}

void ParticleEditorWidget::SwitchEditorToForceMode(SceneEditor2* scene, DAVA::ParticleLayer* layer, DAVA::int32 forceIndex)
{
    ResetEditorMode();

    if (!layer)
    {
        emit ChangeVisible(false);
        return;
    }

    emit ChangeVisible(true);
    widgetMode = MODE_PARTICLE_FORCE;

    layerForceWidget->Init(scene, layer, forceIndex, true);
    setWidget(layerForceWidget);
    layerForceWidget->show();
    connect(layerForceWidget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));

    UpdateParticleEditorWidgets();
}

void ParticleEditorWidget::ResetEditorMode()
{
    QWidget* activeEditorWidget = takeWidget();
    if (!activeEditorWidget)
    {
        DVASSERT(widgetMode == MODE_NONE);
        return;
    }

    switch (widgetMode)
    {
    case MODE_EFFECT:
    {
        effectPropertiesWidget = static_cast<ParticleEffectPropertiesWidget*>(activeEditorWidget);
        effectPropertiesWidget->hide();

        break;
    }

    case MODE_EMITTER:
    {
        emitterPropertiesWidget = static_cast<ParticleEmitterPropertiesWidget*>(activeEditorWidget);
        disconnect(emitterPropertiesWidget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));
        emitterPropertiesWidget->hide();

        break;
    }

    case MODE_LAYER:
    {
        emitterLayerWidget = static_cast<EmitterLayerWidget*>(activeEditorWidget);
        disconnect(emitterLayerWidget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));
        emitterLayerWidget->hide();

        break;
    }

    case MODE_SIMPLIFIED_FORCE:
    {
        layerForceSimplifiedWidget = static_cast<LayerForceSimplifiedWidget*>(activeEditorWidget);
        disconnect(layerForceSimplifiedWidget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));
        layerForceSimplifiedWidget->hide();

        break;
    }

    case MODE_PARTICLE_FORCE:
    {
        layerForceWidget = static_cast<LayerForceWidget*>(activeEditorWidget);
        disconnect(layerForceWidget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));
        layerForceWidget->hide();

        break;
    }

    default:
    {
        break;
    }
    }

    widgetMode = MODE_NONE;
}
