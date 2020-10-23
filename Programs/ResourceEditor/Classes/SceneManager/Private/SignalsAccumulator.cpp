#include "Classes/SceneManager/Private/SignalsAccumulator.h"
#include "Classes/Qt/Scene/SceneSignals.h"

#include <REPlatform/Commands/LandscapeToolsToggleCommand.h>
#include <REPlatform/Commands/ParticleEditorCommands.h>
#include <REPlatform/Commands/ParticleLayerCommands.h>
#include <REPlatform/Commands/RECommandNotificationObject.h>
#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/Systems/HeightmapEditorSystem.h>
#include <REPlatform/Scene/Systems/ModifSystem.h>
#include <REPlatform/Scene/Systems/RulerToolSystem.h>
#include <REPlatform/Scene/Systems/StructureSystem.h>

SignalsAccumulator::SignalsAccumulator(DAVA::SceneEditor2* scene)
{
    scene->commandExecuted.Connect(this, &SignalsAccumulator::OnCommandExecuted);
    scene->GetSystem<DAVA::EntityModificationSystem>()->mouseOverSelection.Connect(this, &SignalsAccumulator::OnMouseOverSelection);
    scene->GetSystem<DAVA::StructureSystem>()->structureChangedSignal.Connect(this, &SignalsAccumulator::OnStructureChanged);
    scene->GetSystem<DAVA::RulerToolSystem>()->rulerToolLengthChanged.Connect(this, &SignalsAccumulator::OnRulerToolLengthChanged);
    scene->GetSystem<DAVA::HeightmapEditorSystem>()->dropperHeightChanged.Connect(this, &SignalsAccumulator::OnDropperHeightChanged);
}

void SignalsAccumulator::OnCommandExecuted(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification)
{
    using namespace DAVA;
    if (commandNotification.MatchCommandTypes<LandscapeToolsToggleCommand>() == true)
    {
        OnLandscapeEditorToggled(scene);
    }

    commandNotification.ForEach<CommandUpdateEmitter>([scene](const CommandUpdateEmitter* cmd) {
        SceneSignals::Instance()->EmitParticleEmitterValueChanged(scene, cmd->GetEmitterInstance());
    });

    commandNotification.ForEach<CommandUpdateParticleLayerBase>([scene](const CommandUpdateParticleLayerBase* cmd) {
        SceneSignals::Instance()->EmitParticleLayerValueChanged(scene, cmd->GetLayer());
    });

    commandNotification.ForEach<CommandChangeLayerMaterialProperties>([scene](const CommandChangeLayerMaterialProperties* cmd) {
        SceneSignals::Instance()->EmitParticleLayerValueChanged(scene, cmd->GetLayer());
    });

    commandNotification.ForEach<CommandChangeFlowProperties>([scene](const CommandChangeFlowProperties* cmd) {
        SceneSignals::Instance()->EmitParticleLayerValueChanged(scene, cmd->GetLayer());
    });

    commandNotification.ForEach<CommandChangeNoiseProperties>([scene](const CommandChangeNoiseProperties* cmd) {
        SceneSignals::Instance()->EmitParticleLayerValueChanged(scene, cmd->GetLayer());
    });

    commandNotification.ForEach<CommandChangeFresnelToAlphaProperties>([scene](const CommandChangeFresnelToAlphaProperties* cmd) {
        SceneSignals::Instance()->EmitParticleLayerValueChanged(scene, cmd->GetLayer());
    });

    commandNotification.ForEach<CommandChangeParticlesStripeProperties>([scene](const CommandChangeParticlesStripeProperties* cmd) {
        SceneSignals::Instance()->EmitParticleLayerValueChanged(scene, cmd->GetLayer());
    });

    commandNotification.ForEach<CommandChangeAlphaRemapProperties>([scene](const CommandChangeAlphaRemapProperties* cmd) {
        SceneSignals::Instance()->EmitParticleLayerValueChanged(scene, cmd->GetLayer());
    });

    commandNotification.ForEach<CommandUpdateParticleLayerBase>([scene](const CommandUpdateParticleLayerBase* cmd) {
        SceneSignals::Instance()->EmitParticleLayerValueChanged(scene, cmd->GetLayer());
    });

    commandNotification.ForEach<CommandUpdateParticleForce>([scene](const CommandUpdateParticleForce* cmd) {
        SceneSignals::Instance()->EmitParticleForceValueChanged(scene, cmd->GetLayer(), cmd->GetForceIndex());
    });

    commandNotification.ForEach<CommandStartStopParticleEffect>([scene](const CommandStartStopParticleEffect* cmd) {
        SceneSignals::Instance()->EmitParticleEffectStateChanged(scene, cmd->GetEntity(), cmd->GetStarted());
    });

    commandNotification.ForEach<CommandRestartParticleEffect>([scene](const CommandRestartParticleEffect* cmd) {
        SceneSignals::Instance()->EmitParticleEffectStateChanged(scene, cmd->GetEntity(), false);
        SceneSignals::Instance()->EmitParticleEffectStateChanged(scene, cmd->GetEntity(), true);
    });

    commandNotification.ForEach<CommandLoadParticleEmitterFromYaml>([scene](const CommandLoadParticleEmitterFromYaml* cmd) {
        SceneSignals::Instance()->EmitParticleEmitterLoaded(scene, cmd->GetEmitterInstance());
    });

    commandNotification.ForEach<CommandSaveParticleEmitterToYaml>([scene](const CommandSaveParticleEmitterToYaml* cmd) {
        SceneSignals::Instance()->EmitParticleEmitterSaved(scene, cmd->GetEmitterInstance());
    });

    commandNotification.ForEach<CommandLoadInnerParticleEmitterFromYaml>([scene](const CommandLoadInnerParticleEmitterFromYaml* cmd) {
        SceneSignals::Instance()->EmitParticleEmitterLoaded(scene, cmd->GetEmitterInstance());
    });

    commandNotification.ForEach<CommandSaveInnerParticleEmitterToYaml>([scene](const CommandSaveInnerParticleEmitterToYaml* cmd) {
        SceneSignals::Instance()->EmitParticleEmitterSaved(scene, cmd->GetEmitterInstance());
    });

    commandNotification.ForEach<CommandAddParticleEmitterLayer>([scene](const CommandAddParticleEmitterLayer* cmd) {
        SceneSignals::Instance()->EmitParticleLayerAdded(scene, cmd->GetParentEmitter(), cmd->GetCreatedLayer());
    });

    SceneSignals::Instance()->EmitCommandExecuted(scene, commandNotification);
}

void SignalsAccumulator::OnStructureChanged(DAVA::SceneEditor2* scene)
{
    SceneSignals::Instance()->EmitStructureChanged(scene, nullptr);
}

void SignalsAccumulator::OnMouseOverSelection(DAVA::SceneEditor2* scene, const DAVA::SelectableGroup* objects)
{
    SceneSignals::Instance()->EmitMouseOverSelection(scene, objects);
}

void SignalsAccumulator::OnDropperHeightChanged(DAVA::SceneEditor2* scene, DAVA::float32 height)
{
    SceneSignals::Instance()->EmitDropperHeightChanged(scene, height);
}

void SignalsAccumulator::OnRulerToolLengthChanged(DAVA::SceneEditor2* scene, DAVA::float64 length, DAVA::float64 previewLength)
{
    SceneSignals::Instance()->EmitRulerToolLengthChanged(scene, length, previewLength);
}

void SignalsAccumulator::OnLandscapeEditorToggled(DAVA::SceneEditor2* scene)
{
    SceneSignals::Instance()->EmitLandscapeEditorToggled(scene);
}
