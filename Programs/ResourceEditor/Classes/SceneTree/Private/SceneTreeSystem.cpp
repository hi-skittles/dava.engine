#include "Classes/SceneTree/Private/SceneTreeSystem.h"

#include <REPlatform/Commands/SetFieldValueCommand.h>
#include <REPlatform/Commands/RECommandNotificationObject.h>
#include <REPlatform/Commands/ParticleEditorCommands.h>
#include <REPlatform/Commands/ParticleEmitterMoveCommands.h>
#include <REPlatform/Commands/ParticleForceMoveCommand.h>
#include <REPlatform/Commands/ParticleLayerMoveCommand.h>

#include <REPlatform/Scene/Systems/EditorParticlesSystem.h>
#include "Particles/ParticleLayer.h"
#include "Particles/ParticleEmitterInstance.h"
#include "Scene3D/Components/ParticleEffectComponent.h"

namespace SceneTreeSystemDetail
{
class ResetSolidFlagForSave : public DAVA::Command
{
public:
    ResetSolidFlagForSave(DAVA::Entity* e)
        : DAVA::Command("")
        , entity(e)
    {
        prevValue = entity->GetSolid();
    }

    void Redo() override
    {
        entity->SetSolid(true);
    }

    void Undo() override
    {
        entity->SetSolid(prevValue);
    }

private:
    DAVA::Entity* entity = nullptr;
    bool prevValue = false;
};

DAVA::uint32 CalcEntityDepth(DAVA::Entity* e)
{
    DAVA::uint32 depth = 0;
    while (e != nullptr)
    {
        ++depth;
        e = e->GetParent();
    }

    return depth;
}

DAVA::int32 CalcParticleElementsDepth(DAVA::ParticleEffectComponent* component, DAVA::BaseObject* wantedObject)
{
    const DAVA::ReflectedType* objType = DAVA::GetValueReflectedType(DAVA::Any(wantedObject));
    bool isForce = objType == DAVA::ReflectedTypeDB::Get<DAVA::ParticleForce>();
    bool isSimplifiedForce = objType == DAVA::ReflectedTypeDB::Get<DAVA::ParticleForceSimplified>();
    bool isLayer = objType == DAVA::ReflectedTypeDB::Get<DAVA::ParticleLayer>();
    bool isEmitter = objType == DAVA::ReflectedTypeDB::Get<DAVA::ParticleEmitterInstance>();

#if defined(__DAVAENGINE_DEBUG__)
    auto fn = [](bool v) { return v == true ? 1 : 0; };
    DVASSERT((fn(isForce) + fn(isLayer) + fn(isEmitter)) == 1);
#endif

    DAVA::Stack<DAVA::BaseObject*> objectsPath;
    DAVA::Function<bool(DAVA::ParticleLayer*)> lookupLayer = [&](DAVA::ParticleLayer* layer) -> bool {
        if (isLayer == true)
        {
            if (layer == wantedObject)
            {
                return true;
            }
        }

        if (isForce == true)
        {
            for (DAVA::ParticleForce* force : layer->GetParticleForces())
            {
                if (force == wantedObject)
                {
                    objectsPath.push(force);
                    return true;
                }
            }
        }

        if (isSimplifiedForce == true)
        {
            for (DAVA::ParticleForceSimplified* force : layer->GetSimplifiedParticleForces())
            {
                if (force == wantedObject)
                {
                    objectsPath.push(force);
                    return true;
                }
            }
        }

        if (layer->type == DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
        {
            DVASSERT(layer->innerEmitter != nullptr);
            objectsPath.push(layer->innerEmitter);
            if (isEmitter == true && layer->innerEmitter == wantedObject)
            {
                return true;
            }

            for (DAVA::ParticleLayer* layer : layer->innerEmitter->GetEmitter()->layers)
            {
                objectsPath.push(layer);
                if (lookupLayer(layer) == true)
                {
                    return true;
                }
                objectsPath.pop();
            }
            objectsPath.pop();
        }

        return false;
    };

    for (DAVA::uint32 i = 0; i < component->GetEmittersCount(); ++i)
    {
        DAVA::ParticleEmitterInstance* instance = component->GetEmitterInstance(i);
        objectsPath.push(instance);
        if (instance == wantedObject)
        {
            return static_cast<DAVA::int32>(objectsPath.size());
        }

        for (DAVA::ParticleLayer* layer : instance->GetEmitter()->layers)
        {
            objectsPath.push(layer);
            if (lookupLayer(layer) == true)
            {
                return static_cast<DAVA::int32>(objectsPath.size());
            }
            objectsPath.pop();
        }

        objectsPath.pop();
    }

    DVASSERT(isEmitter == true);
    return 0;
}

} // namespace SceneTreeSystemDetail

SceneTreeSystem::SceneTreeSystem(DAVA::Scene* scene)
    : SceneSystem(scene)
{
}

void SceneTreeSystem::RegisterEntity(DAVA::Entity* entity)
{
    using namespace SceneTreeSystemDetail;
    if (entity == GetScene())
    {
        return;
    }

    entity->SetSolid(true);
    if (IsSystemEnabled() == false)
    {
        return;
    }

    DAVA::Entity* parentEntity = entity->GetParent();
    if (parentEntity != nullptr)
    {
        syncSnapshot.objectsToRefetch[CalcEntityDepth(parentEntity)].push_back(DAVA::Selectable(DAVA::Any(parentEntity)));
    }
}

void SceneTreeSystem::RegisterComponent(DAVA::Entity* entity, DAVA::Component* component)
{
    if (IsSystemEnabled() == false)
    {
        return;
    }

    if (entity == GetScene())
    {
        return;
    }
    syncSnapshot.changedObjects.emplace(DAVA::Selectable(DAVA::Any(entity)));
}

void SceneTreeSystem::UnregisterEntity(DAVA::Entity* entity)
{
    using namespace SceneTreeSystemDetail;
    if (IsSystemEnabled() == false)
    {
        return;
    }

    if (entity == GetScene())
    {
        return;
    }

    DAVA::Selectable obj = DAVA::Selectable(DAVA::Any(entity));

    syncSnapshot.changedObjects.erase(obj);
    syncSnapshot.removedObjects[CalcEntityDepth(entity)].push_back(obj);
}

void SceneTreeSystem::UnregisterComponent(DAVA::Entity* entity, DAVA::Component* component)
{
    if (IsSystemEnabled() == false)
    {
        return;
    }

    if (entity == GetScene())
    {
        return;
    }
    syncSnapshot.changedObjects.emplace(DAVA::Selectable(DAVA::Any(entity)));
}

void SceneTreeSystem::PrepareForRemove()
{
    SyncFinished();
}

void SceneTreeSystem::Process(DAVA::float32 timeElapsed)
{
    if (syncRequested == false && syncSnapshot.IsEmpty() == false)
    {
        syncIsNecessary.Emit();
        syncRequested = true;
    }
}

void SceneTreeSystem::ProcessCommand(const DAVA::RECommandNotificationObject& commandNotification)
{
    using namespace SceneTreeSystemDetail;
    if (IsSystemEnabled() == false)
    {
        return;
    }

    commandNotification.ForEach<DAVA::SetFieldValueCommand>([&](const DAVA::SetFieldValueCommand* command) {
        const DAVA::Reflection::Field& f = command->GetField();
        if (f.key.CanCast<DAVA::FastName>() && f.key.Cast<DAVA::FastName>() == DAVA::Entity::EntityNameFieldName)
        {
            DAVA::ReflectedObject obj = f.ref.GetDirectObject();
            if (obj.GetReflectedType() == DAVA::ReflectedTypeDB::Get<DAVA::Entity>())
            {
                syncSnapshot.changedObjects.insert(DAVA::Selectable(obj.GetPtr<DAVA::Entity>()));
            }
        }
    });

    commandNotification.ForEach<DAVA::CommandAddParticleEmitter>([&](const DAVA::CommandAddParticleEmitter* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandAddParticleEmitter>::value, "You should support undo for this command here");
        DAVA::Entity* entity = command->GetEntity();
        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity)].push_back(DAVA::Selectable(DAVA::Any(entity)));
    });

    commandNotification.ForEach<DAVA::CommandRemoveParticleEmitter>([&](const DAVA::CommandRemoveParticleEmitter* command) {
        DAVA::ParticleEffectComponent* component = command->GetEffect();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleEmitterInstance* emitterInstance = command->GetEmitterInstance();
        if (commandNotification.IsRedo() == true)
        {
            syncSnapshot.removedObjects[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, emitterInstance) + 1].push_back(DAVA::Selectable(DAVA::Any(emitterInstance)));
        }
        else
        {
            syncSnapshot.objectsToRefetch[CalcEntityDepth(entity)].push_back(DAVA::Selectable(DAVA::Any(entity)));
        }
    });

    commandNotification.ForEach<DAVA::CommandUpdateEmitter>([&](const DAVA::CommandUpdateEmitter* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandUpdateEmitter>::value, "You should support undo for this command here");
        DAVA::ParticleEmitterInstance* emitterInstance = command->GetEmitterInstance();
        syncSnapshot.changedObjects.insert(DAVA::Selectable(DAVA::Any(emitterInstance)));
    });

    commandNotification.ForEach<DAVA::ParticleEmitterMoveCommand>([&](const DAVA::ParticleEmitterMoveCommand* command) {
        DAVA::Entity* oldEntity = command->GetOldComponent()->GetEntity();
        DAVA::Entity* newEntity = command->GetNewComponent()->GetEntity();
        DAVA::ParticleEmitterInstance* instance = command->GetEmitterInstance();

        if (commandNotification.IsRedo() == true)
        {
            syncSnapshot.objectsToRefetch[CalcEntityDepth(newEntity)].push_back(DAVA::Selectable(DAVA::Any(newEntity)));
            syncSnapshot.removedObjects[CalcEntityDepth(oldEntity) + 1].push_back(DAVA::Selectable(DAVA::Any(instance)));
        }
        else
        {
            syncSnapshot.objectsToRefetch[CalcEntityDepth(oldEntity)].push_back(DAVA::Selectable(DAVA::Any(oldEntity)));
            syncSnapshot.removedObjects[CalcEntityDepth(newEntity) + 1].push_back(DAVA::Selectable(DAVA::Any(instance)));
        }
    });

    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    commandNotification.ForEach<DAVA::CommandAddParticleEmitterLayer>([&](const DAVA::CommandAddParticleEmitterLayer* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandAddParticleEmitterLayer>::value, "You should support undo for this command here");
        DAVA::ParticleEffectComponent* component = command->GetEffectComponent();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleEmitterInstance* instance = command->GetParentEmitter();

        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, instance) + 1].push_back(DAVA::Selectable(DAVA::Any(instance)));
    });

    commandNotification.ForEach<DAVA::CommandCloneParticleEmitterLayer>([&](const DAVA::CommandCloneParticleEmitterLayer* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandCloneParticleEmitterLayer>::value, "You should support undo for this command here");
        DAVA::ParticleEmitterInstance* instance = command->GetEmitterInstance();
        DAVA::ParticleEffectComponent* component = GetScene()->GetSystem<DAVA::EditorParticlesSystem>()->GetEmitterOwner(instance);
        DAVA::Entity* entity = component->GetEntity();

        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, instance) + 1].push_back(DAVA::Selectable(DAVA::Any(instance)));
    });

    commandNotification.ForEach<DAVA::ParticleLayerMoveCommand>([&](const DAVA::ParticleLayerMoveCommand* command) {
        DAVA::ParticleLayer* layer = command->GetLayer();
        DAVA::EditorParticlesSystem* system = GetScene()->GetSystem<DAVA::EditorParticlesSystem>();
        DAVA::ParticleEmitterInstance* oldEmitter = command->GetOldEmitter();
        DAVA::ParticleEmitterInstance* newEmitter = command->GetNewEmitter();
        DAVA::ParticleEffectComponent* oldComponent = system->GetEmitterOwner(oldEmitter);
        DAVA::ParticleEffectComponent* newComponent = system->GetEmitterOwner(newEmitter);

        if (commandNotification.IsRedo() == true)
        {
            syncSnapshot.removedObjects[CalcEntityDepth(oldComponent->GetEntity()) + CalcParticleElementsDepth(oldComponent, oldEmitter) + 2].push_back(DAVA::Selectable(DAVA::Any(layer)));
            syncSnapshot.objectsToRefetch[CalcEntityDepth(newComponent->GetEntity()) + CalcParticleElementsDepth(newComponent, newEmitter) + 1].push_back(DAVA::Selectable(DAVA::Any(newEmitter)));
        }
        else
        {
            syncSnapshot.removedObjects[CalcEntityDepth(newComponent->GetEntity()) + CalcParticleElementsDepth(newComponent, newEmitter) + 2].push_back(DAVA::Selectable(DAVA::Any(layer)));
            syncSnapshot.objectsToRefetch[CalcEntityDepth(oldComponent->GetEntity()) + CalcParticleElementsDepth(oldComponent, oldEmitter) + 1].push_back(DAVA::Selectable(DAVA::Any(oldEmitter)));
        }
    });

    commandNotification.ForEach<DAVA::CommandUpdateParticleLayer>([&](const DAVA::CommandUpdateParticleLayer* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandUpdateParticleLayer>::value, "You should support undo for this command here");

        DAVA::EditorParticlesSystem* system = GetScene()->GetSystem<DAVA::EditorParticlesSystem>();

        DAVA::ParticleLayer* layer = command->GetLayer();
        DAVA::ParticleEffectComponent* component = system->GetEmitterOwner(system->GetRootEmitterLayerOwner(layer));
        DAVA::ParticleEmitterInstance* deletedEmitter = command->GetDeletedEmitter();
        DAVA::ParticleEmitterInstance* createdEmitter = command->GetCreatedEmitter();

        DAVA::int32 layerDepth = CalcEntityDepth(component->GetEntity()) + CalcParticleElementsDepth(component, layer) + 1;

        syncSnapshot.changedObjects.insert(DAVA::Selectable(DAVA::Any(layer)));
        if (deletedEmitter != nullptr)
        {
            syncSnapshot.removedObjects[layerDepth + 1].push_back(DAVA::Selectable(DAVA::Any(deletedEmitter)));
        }

        if (createdEmitter != nullptr)
        {
            syncSnapshot.objectsToRefetch[layerDepth].push_back(DAVA::Selectable(DAVA::Any(layer)));
        }
    });

    commandNotification.ForEach<DAVA::CommandUpdateParticleLayerEnabled>([&](const DAVA::CommandUpdateParticleLayerEnabled* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandUpdateParticleLayerEnabled>::value, "You should support undo for this command here");
        syncSnapshot.changedObjects.insert(DAVA::Selectable(DAVA::Any(command->GetLayer())));
    });

    commandNotification.ForEach<DAVA::CommandRemoveParticleEmitterLayer>([&](const DAVA::CommandRemoveParticleEmitterLayer* command) {
        DAVA::ParticleEffectComponent* component = command->GetEffectComponent();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleEmitterInstance* emitterInstance = command->GetEmitterInstance();

        if (commandNotification.IsRedo() == true)
        {
            syncSnapshot.removedObjects[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, emitterInstance) + 2].push_back(DAVA::Selectable(DAVA::Any(command->GetLayer())));
        }
        else
        {
            syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, emitterInstance) + 1].push_back(DAVA::Selectable(DAVA::Any(emitterInstance)));
        }
    });

    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    commandNotification.ForEach<DAVA::CommandAddParticleEmitterSimplifiedForce>([&](const DAVA::CommandAddParticleEmitterSimplifiedForce* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandAddParticleEmitterSimplifiedForce>::value, "You should support undo for this command here");
        DAVA::ParticleEffectComponent* component = command->GetEffectComponent();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleLayer* layer = command->GetLayer();

        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 1].push_back(DAVA::Selectable(DAVA::Any(layer)));
    });

    commandNotification.ForEach<DAVA::CommandRemoveParticleEmitterSimplifiedForce>([&](const DAVA::CommandRemoveParticleEmitterSimplifiedForce* command) {
        DAVA::ParticleEffectComponent* component = command->GetEffectComponent();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleLayer* layer = command->GetLayer();

        if (commandNotification.IsRedo() == true)
        {
            syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 1].push_back(DAVA::Selectable(DAVA::Any(layer)));
            syncSnapshot.removedObjects[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 2].push_back(DAVA::Selectable(DAVA::Any(command->GetForce())));
        }
        else
        {
            syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 1].push_back(DAVA::Selectable(DAVA::Any(layer)));
            syncSnapshot.removedObjects[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 2].push_back(DAVA::Selectable(DAVA::Any(command->GetForce())));
        }
    });

    commandNotification.ForEach<DAVA::ParticleSimplifiedForceMoveCommand>([&](const DAVA::ParticleSimplifiedForceMoveCommand* command) {
        DAVA::EditorParticlesSystem* system = GetScene()->GetSystem<DAVA::EditorParticlesSystem>();
        DAVA::ParticleEffectComponent* oldComponent = system->GetRootEmitterLayerOwner(command->oldLayer)->GetOwner();
        DAVA::ParticleEffectComponent* newComponent = system->GetRootEmitterLayerOwner(command->newLayer)->GetOwner();

        if (commandNotification.IsRedo() == true)
        {
            syncSnapshot.objectsToRefetch[CalcEntityDepth(newComponent->GetEntity()) + CalcParticleElementsDepth(newComponent, command->newLayer) + 1].push_back(DAVA::Selectable(DAVA::Any(command->newLayer)));
            syncSnapshot.removedObjects[CalcEntityDepth(oldComponent->GetEntity()) + CalcParticleElementsDepth(oldComponent, command->oldLayer) + 2].push_back(DAVA::Selectable(DAVA::Any(command->force)));
        }
        else
        {
            syncSnapshot.objectsToRefetch[CalcEntityDepth(oldComponent->GetEntity()) + CalcParticleElementsDepth(oldComponent, command->oldLayer) + 1].push_back(DAVA::Selectable(DAVA::Any(command->oldLayer)));
            syncSnapshot.removedObjects[CalcEntityDepth(newComponent->GetEntity()) + CalcParticleElementsDepth(newComponent, command->newLayer) + 2].push_back(DAVA::Selectable(DAVA::Any(command->force)));
        }
    });

    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    commandNotification.ForEach<DAVA::ParticleForceMoveCommand>([&](const DAVA::ParticleForceMoveCommand* command) {
        DAVA::EditorParticlesSystem* system = GetScene()->GetSystem<DAVA::EditorParticlesSystem>();
        DAVA::ParticleEffectComponent* oldComponent = system->GetRootEmitterLayerOwner(command->oldLayer)->GetOwner();
        DAVA::ParticleEffectComponent* newComponent = system->GetRootEmitterLayerOwner(command->newLayer)->GetOwner();

        if (commandNotification.IsRedo() == true)
        {
            syncSnapshot.objectsToRefetch[CalcEntityDepth(newComponent->GetEntity()) + CalcParticleElementsDepth(newComponent, command->newLayer) + 1].push_back(DAVA::Selectable(DAVA::Any(command->newLayer)));
            syncSnapshot.removedObjects[CalcEntityDepth(oldComponent->GetEntity()) + CalcParticleElementsDepth(oldComponent, command->oldLayer) + 2].push_back(DAVA::Selectable(DAVA::Any(command->force)));
        }
        else
        {
            syncSnapshot.objectsToRefetch[CalcEntityDepth(oldComponent->GetEntity()) + CalcParticleElementsDepth(oldComponent, command->oldLayer) + 1].push_back(DAVA::Selectable(DAVA::Any(command->oldLayer)));
            syncSnapshot.removedObjects[CalcEntityDepth(newComponent->GetEntity()) + CalcParticleElementsDepth(newComponent, command->newLayer) + 2].push_back(DAVA::Selectable(DAVA::Any(command->force)));
        }
    });

    commandNotification.ForEach<DAVA::CommandAddParticleDrag>([&](const DAVA::CommandAddParticleDrag* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandAddParticleDrag>::value, "You should support undo for this command here");
        DAVA::ParticleEffectComponent* component = command->GetEffectComponent();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleLayer* layer = command->GetLayer();

        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 1].push_back(DAVA::Selectable(DAVA::Any(layer)));
    });

    commandNotification.ForEach<DAVA::CommandAddParticleVortex>([&](const DAVA::CommandAddParticleVortex* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandAddParticleVortex>::value, "You should support undo for this command here");
        DAVA::ParticleEffectComponent* component = command->GetEffectComponent();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleLayer* layer = command->GetLayer();

        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 1].push_back(DAVA::Selectable(DAVA::Any(layer)));
    });

    commandNotification.ForEach<DAVA::CommandAddParticleGravity>([&](const DAVA::CommandAddParticleGravity* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandAddParticleGravity>::value, "You should support undo for this command here");
        DAVA::ParticleEffectComponent* component = command->GetEffectComponent();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleLayer* layer = command->GetLayer();

        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 1].push_back(DAVA::Selectable(DAVA::Any(layer)));
    });

    commandNotification.ForEach<DAVA::CommandAddParticleWind>([&](const DAVA::CommandAddParticleWind* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandAddParticleWind>::value, "You should support undo for this command here");
        DAVA::ParticleEffectComponent* component = command->GetEffectComponent();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleLayer* layer = command->GetLayer();

        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 1].push_back(DAVA::Selectable(DAVA::Any(layer)));
    });

    commandNotification.ForEach<DAVA::CommandAddParticlePointGravity>([&](const DAVA::CommandAddParticlePointGravity* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandAddParticlePointGravity>::value, "You should support undo for this command here");
        DAVA::ParticleEffectComponent* component = command->GetEffectComponent();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleLayer* layer = command->GetLayer();

        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 1].push_back(DAVA::Selectable(DAVA::Any(layer)));
    });

    commandNotification.ForEach<DAVA::CommandAddParticlePlaneCollision>([&](const DAVA::CommandAddParticlePlaneCollision* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandAddParticlePlaneCollision>::value, "You should support undo for this command here");
        DAVA::ParticleEffectComponent* component = command->GetEffectComponent();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleLayer* layer = command->GetLayer();

        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 1].push_back(DAVA::Selectable(DAVA::Any(layer)));
    });

    commandNotification.ForEach<DAVA::CommandCloneParticleForce>([&](const DAVA::CommandCloneParticleForce* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandCloneParticleForce>::value, "You should support undo for this command here");
        DAVA::ParticleEffectComponent* component = command->GetEffectComponent();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleLayer* layer = command->GetLayer();

        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 1].push_back(DAVA::Selectable(DAVA::Any(layer)));
    });

    commandNotification.ForEach<DAVA::CommandUpdateParticleForce>([&](const DAVA::CommandUpdateParticleForce* command) {
        static_assert(std::is_base_of<DAVA::CommandAction, DAVA::CommandUpdateParticleForce>::value, "You should support undo for this command here");
        DAVA::ParticleLayer* layer = command->GetLayer();
        DAVA::ParticleForce* force = layer->GetParticleForces()[command->GetForceIndex()];

        syncSnapshot.changedObjects.insert(DAVA::Selectable(DAVA::Any(force)));
    });

    commandNotification.ForEach<DAVA::CommandRemoveParticleForce>([&](const DAVA::CommandRemoveParticleForce* command) {
        DAVA::ParticleEffectComponent* component = command->GetEffectComponent();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleLayer* layer = command->GetLayer();

        if (commandNotification.IsRedo() == true)
        {
            syncSnapshot.removedObjects[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 2].push_back(DAVA::Selectable(DAVA::Any(command->GetForce())));
        }
        else
        {
            syncSnapshot.objectsToRefetch[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, layer) + 1].push_back(DAVA::Selectable(DAVA::Any(layer)));
        }
    });

    commandNotification.ForEach<DAVA::CommandLoadParticleEmitterFromYaml>([&](const DAVA::CommandLoadParticleEmitterFromYaml* command) {
        DAVA::ParticleEffectComponent* component = command->GetEffect();
        DAVA::Entity* entity = component->GetEntity();
        DAVA::ParticleEmitterInstance* emitter = command->GetEmitterInstance();

        syncSnapshot.removedObjects[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, emitter) + 1].push_back(DAVA::Selectable(DAVA::Any(emitter)));
        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity)].push_back(DAVA::Selectable(DAVA::Any(entity)));
    });

    commandNotification.ForEach<DAVA::CommandLoadInnerParticleEmitterFromYaml>([&](const DAVA::CommandLoadInnerParticleEmitterFromYaml* command) {
        DAVA::ParticleEmitterInstance* emitter = command->GetEmitterInstance();
        DAVA::ParticleEffectComponent* component = GetScene()->GetSystem<DAVA::EditorParticlesSystem>()->GetEmitterOwner(emitter);
        DAVA::Entity* entity = component->GetEntity();

        syncSnapshot.removedObjects[CalcEntityDepth(entity) + CalcParticleElementsDepth(component, emitter) + 1].push_back(DAVA::Selectable(DAVA::Any(emitter)));
        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity)].push_back(DAVA::Selectable(DAVA::Any(entity)));
    });

    commandNotification.ForEach<DAVA::CommandReloadEmitters>([&](const DAVA::CommandReloadEmitters* command) {
        DAVA::ParticleEffectComponent* component = command->GetComponent();
        DAVA::Entity* entity = component->GetEntity();

        const DAVA::Vector<DAVA::ParticleEmitterInstance*>* redoEmitters = &command->GetRedoEmitters();
        const DAVA::Vector<DAVA::ParticleEmitterInstance*>* undoEmitters = &command->GetUndoEmitters();

        if (commandNotification.IsRedo() == false)
        {
            std::swap(redoEmitters, undoEmitters);
        }

        for (DAVA::ParticleEmitterInstance* emitter : (*undoEmitters))
        {
            syncSnapshot.removedObjects[CalcEntityDepth(entity) + 1].push_back(DAVA::Selectable(DAVA::Any(emitter)));
        }

        syncSnapshot.objectsToRefetch[CalcEntityDepth(entity)].push_back(DAVA::Selectable(DAVA::Any(entity)));
    });
}

const SceneTreeSystem::SyncSnapshot& SceneTreeSystem::GetSyncSnapshot() const
{
    return syncSnapshot;
}

void SceneTreeSystem::SyncFinished()
{
    syncSnapshot = SyncSnapshot();
    syncRequested = false;
}

std::unique_ptr<DAVA::Command> SceneTreeSystem::PrepareForSave(bool saveForGame)
{
    DAVA::CommandBatch* batch = new DAVA::CommandBatch();
    DAVA::Scene* scene = GetScene();

    DAVA::Function<void(DAVA::Entity*)> fn = [batch, &fn](DAVA::Entity* e) {
        batch->Add(std::make_unique<SceneTreeSystemDetail::ResetSolidFlagForSave>(e));
        for (DAVA::int32 i = 0; i < e->GetChildrenCount(); ++i)
        {
            fn(e->GetChild(i));
        }
    };

    fn(scene);
    return std::unique_ptr<DAVA::Command>(batch);
}
