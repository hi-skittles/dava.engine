#pragma once


#include <REPlatform/DataNodes/SelectableGroup.h>
#include <REPlatform/Scene/SceneEditor2.h>

#include <Base/StaticSingleton.h>
#include <Scene3D/Entity.h>

#include <QObject>

namespace DAVA
{
class ParticleEmitterInstance;
struct ParticleLayer;
class SceneEditor2;
class RECommandNotificationObject;
}

class SceneSignals : public QObject, public DAVA::StaticSingleton<SceneSignals>
{
    Q_OBJECT

signals:

    // scene
    void Opened(DAVA::SceneEditor2* scene); //
    void Closed(DAVA::SceneEditor2* scene); //

    void Activated(DAVA::SceneEditor2* scene); //
    void Deactivated(DAVA::SceneEditor2* scene); //
    void CommandExecuted(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification);
    void StructureChanged(DAVA::SceneEditor2* scene, DAVA::Entity* parent);

    // Quality
    void QualityChanged();

    // mouse
    void MouseOverSelection(DAVA::SceneEditor2* scene, const DAVA::SelectableGroup* objects);

    // particles - value changed
    void ParticleEmitterValueChanged(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter);
    void ParticleLayerValueChanged(DAVA::SceneEditor2* scene, DAVA::ParticleLayer* layer);
    void ParticleForceValueChanged(DAVA::SceneEditor2* scene, DAVA::ParticleLayer* layer, DAVA::int32 forceIndex);
    void ParticleDragForceValueChanged(DAVA::SceneEditor2* scene, DAVA::ParticleLayer* layer, DAVA::int32 forceIndex);

    // particles - effect started/stopped.
    void ParticleEffectStateChanged(DAVA::SceneEditor2* scene, DAVA::Entity* effect, bool isStarted);

    // particles - loading/saving.
    void ParticleEmitterLoaded(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter);
    void ParticleEmitterSaved(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter);

    // particles - structure changes.
    void ParticleLayerAdded(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter, DAVA::ParticleLayer* layer);
    void ParticleLayerRemoved(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter);

    void DropperHeightChanged(DAVA::SceneEditor2* scene, double height);
    void RulerToolLengthChanged(DAVA::SceneEditor2* scene, double length, double previewLength);

    void LandscapeEditorToggled(DAVA::SceneEditor2* scene);

public:
    void EmitOpened(DAVA::SceneEditor2* scene)
    {
        emit Opened(scene);
    }
    void EmitClosed(DAVA::SceneEditor2* scene)
    {
        emit Closed(scene);
    }

    void EmitActivated(DAVA::SceneEditor2* scene)
    {
        emit Activated(scene);
    }
    void EmitDeactivated(DAVA::SceneEditor2* scene)
    {
        emit Deactivated(scene);
    }

    void EmitCommandExecuted(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification)
    {
        emit CommandExecuted(scene, commandNotification);
    };

    void EmitStructureChanged(DAVA::SceneEditor2* scene, DAVA::Entity* parent)
    {
        emit StructureChanged(scene, parent);
    }

    void EmitQualityChanged()
    {
        emit QualityChanged();
    }

    void EmitLandscapeEditorToggled(DAVA::SceneEditor2* scene)
    {
        emit LandscapeEditorToggled(scene);
    }

    void EmitDropperHeightChanged(DAVA::SceneEditor2* scene, DAVA::float32 height)
    {
        emit DropperHeightChanged(scene, (double)height);
    };

    void EmitRulerToolLengthChanged(DAVA::SceneEditor2* scene, double length, double previewLength)
    {
        emit RulerToolLengthChanged(scene, length, previewLength);
    }

    void EmitMouseOverSelection(DAVA::SceneEditor2* scene, const DAVA::SelectableGroup* objects)
    {
        emit MouseOverSelection(scene, objects);
    }

    // Particle Editor Value Changed signals.
    void EmitParticleEmitterValueChanged(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter)
    {
        emit ParticleEmitterValueChanged(scene, emitter);
    }

    void EmitParticleLayerValueChanged(DAVA::SceneEditor2* scene, DAVA::ParticleLayer* layer)
    {
        emit ParticleLayerValueChanged(scene, layer);
    }

    void EmitParticleForceValueChanged(DAVA::SceneEditor2* scene, DAVA::ParticleLayer* layer, DAVA::int32 forceIndex)
    {
        emit ParticleForceValueChanged(scene, layer, forceIndex);
    }

    void EmitParticleDragForceValueChanged(DAVA::SceneEditor2* scene, DAVA::ParticleLayer* layer, DAVA::int32 forceIndex)
    {
        emit ParticleDragForceValueChanged(scene, layer, forceIndex);
    }

    void EmitParticleEffectStateChanged(DAVA::SceneEditor2* scene, DAVA::Entity* effect, bool isStarted)
    {
        emit ParticleEffectStateChanged(scene, effect, isStarted);
    }

    void EmitParticleEmitterLoaded(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter)
    {
        emit ParticleEmitterLoaded(scene, emitter);
    }

    void EmitParticleEmitterSaved(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter)
    {
        emit ParticleEmitterSaved(scene, emitter);
    }

    void EmitParticleLayerAdded(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter, DAVA::ParticleLayer* layer)
    {
        emit ParticleLayerAdded(scene, emitter, layer);
    }

    void EmitParticleLayerRemoved(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter)
    {
        emit ParticleLayerRemoved(scene, emitter);
    }
};
