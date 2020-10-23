#pragma once

#include "Classes/Selection/SelectableGroup.h"
#include "StringConstants.h"
#include "SystemDelegates.h"
#include "Classes/Qt/Scene/System/EditorSceneSystem.h"

// framework
#include "Entity/SceneSystem.h"
#include "Scene3D/Entity.h"
#include "Particles/ParticleLayer.h"
#include "Particles/ParticleEmitter.h"
#include "Scene3D/Components/ParticleEffectComponent.h"
#include "UI/UIEvent.h"
#include "Render/Highlevel/Landscape.h"
#include "Functional/Function.h"

class RECommandNotificationObject;
class StructureSystem : public DAVA::SceneSystem, public EditorSceneSystem
{
public:
    using InternalMapping = DAVA::Map<DAVA::Entity*, DAVA::Entity*>;

public:
    StructureSystem(DAVA::Scene* scene);
    ~StructureSystem();

    void Process(DAVA::float32 timeElapsed) override;

    void Move(const SelectableGroup& objects, DAVA::Entity* newParent, DAVA::Entity* newBefore);
    void MoveEmitter(const DAVA::Vector<DAVA::ParticleEmitterInstance*>& emitters, const DAVA::Vector<DAVA::ParticleEffectComponent*>& oldEffects, DAVA::ParticleEffectComponent* newEffect, int dropAfter);
    void MoveLayer(const DAVA::Vector<DAVA::ParticleLayer*>& layers, const DAVA::Vector<DAVA::ParticleEmitterInstance*>& oldEmitters, DAVA::ParticleEmitterInstance* newEmitter, DAVA::ParticleLayer* newBefore);
    void MoveSimplifiedForce(const DAVA::Vector<DAVA::ParticleForceSimplified*>& forces, const DAVA::Vector<DAVA::ParticleLayer*>& oldLayers, DAVA::ParticleLayer* newLayer);
    void MoveParticleForce(const DAVA::Vector<DAVA::ParticleForce*>& forces, const DAVA::Vector<DAVA::ParticleLayer*>& oldLayers, DAVA::ParticleLayer* newLayer);

    void Remove(const SelectableGroup& objects);

    SelectableGroup ReloadEntities(const SelectableGroup& objects, bool saveLightmapSettings = false);

    // Mapping is link between old entity and new entity
    void ReloadRefs(const DAVA::FilePath& modelPath, InternalMapping& mapping, bool saveLightmapSettings = false);
    SelectableGroup ReloadEntitiesAs(const SelectableGroup& objects, const DAVA::FilePath& newModelPath, bool saveLightmapSettings = false);
    void Add(const DAVA::FilePath& newModelPath, const DAVA::Vector3 pos = DAVA::Vector3());

    void EmitChanged();

    DAVA::Entity* Load(const DAVA::FilePath& sc2path);

    void AddDelegate(StructureSystemDelegate* delegate);
    void RemoveDelegate(StructureSystemDelegate* delegate);

    void CheckAndMarkSolid(DAVA::Entity* entity);

protected:
    void ProcessCommand(const RECommandNotificationObject& commandNotification) override;

private:
    void AddEntity(DAVA::Entity* entity) override;
    void RemoveEntity(DAVA::Entity* entity) override;
    void PrepareForRemove() override;

    void ReloadInternal(InternalMapping& mapping, const DAVA::FilePath& newModelPath, bool saveLightmapSettings);
    DAVA::Entity* LoadInternal(const DAVA::FilePath& sc2path, bool clearCached);

    bool CopyLightmapSettings(DAVA::Entity* fromState, DAVA::Entity* toState) const;
    void CopyLightmapSettings(DAVA::NMaterial* fromEntity, DAVA::NMaterial* toEntity) const;
    void FindMeshesRecursive(DAVA::Entity* entity, DAVA::Vector<DAVA::RenderObject*>& objects) const;

    void SearchEntityByRef(DAVA::Entity* parent, const DAVA::FilePath& refToOwner, const DAVA::Function<void(DAVA::Entity*)>& callback);

    void RemoveEntities(DAVA::Vector<DAVA::Entity*>& entitiesToRemove);

    DAVA::List<StructureSystemDelegate*> delegates;
    bool structureChanged = false;
};
