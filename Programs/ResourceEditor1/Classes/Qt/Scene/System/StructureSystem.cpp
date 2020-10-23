#include "Scene/System/StructureSystem.h"
#include "Scene/System/CameraSystem.h"
#include "Scene/SceneSignals.h"
#include "Scene/SceneEditor2.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/Project/ProjectManagerData.h"

#include "Commands2/Base/RECommandNotificationObject.h"
#include "Commands2/EntityParentChangeCommand.h"
#include "Commands2/EntityAddCommand.h"
#include "Commands2/EntityRemoveCommand.h"
#include "Commands2/ParticleEmitterMoveCommands.h"
#include "Commands2/ParticleLayerMoveCommand.h"
#include "Commands2/ParticleLayerRemoveCommand.h"
#include "Commands2/ParticleForceMoveCommand.h"
#include "Commands2/ParticleForceSimplifiedRemoveCommand.h"

#include "Deprecated/SceneValidator.h"

namespace StructSystemDetails
{
void MapSelectableGroup(const SelectableGroup& srcGroup, SelectableGroup& dstGroup,
                        const StructureSystem::InternalMapping& mapping, SceneCollisionSystem* collisionSystem)
{
    DVASSERT(collisionSystem != nullptr);

    for (auto entity : srcGroup.ObjectsOfType<DAVA::Entity>())
    {
        auto i = mapping.find(entity);
        if (i != mapping.end())
        {
            dstGroup.Add(i->first);
        }
    }
}

struct SortEntity
{
    SortEntity(DAVA::Entity* entity, DAVA::uint32 hierarchyIndex)
        : entity(entity)
        , hierarchyIndex(hierarchyIndex)
    {
    }

    DAVA::Entity* entity = nullptr;
    DAVA::uint32 hierarchyIndex = 0;

    bool operator<(const SortEntity& e) const
    {
        return hierarchyIndex < e.hierarchyIndex;
    }
};
}

StructureSystem::StructureSystem(DAVA::Scene* scene)
    : DAVA::SceneSystem(scene)
{
}

StructureSystem::~StructureSystem()
{
}

void StructureSystem::Move(const SelectableGroup& objects, DAVA::Entity* newParent, DAVA::Entity* newBefore)
{
    SceneEditor2* sceneEditor = (SceneEditor2*)GetScene();
    const auto& objectsContent = objects.GetContent();
    if ((sceneEditor == nullptr) || objectsContent.empty())
        return;

    sceneEditor->BeginBatch("Move entities", objects.GetSize());
    for (auto entity : objects.ObjectsOfType<DAVA::Entity>())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new EntityParentChangeCommand(entity, newParent, newBefore)));
    }
    sceneEditor->EndBatch();
    EmitChanged();
}

void StructureSystem::RemoveEntities(DAVA::Vector<DAVA::Entity*>& objects)
{
    // sort objects by parents (even if parent == nullptr), in order to remove children first
    std::priority_queue<StructSystemDetails::SortEntity> queue;
    for (DAVA::Entity* entity : objects)
    {
        DAVA::uint32 hierarchyIndex = 0;
        DAVA::Entity* e = entity;
        while (e->GetParent() != nullptr)
        {
            ++hierarchyIndex;
            e = e->GetParent();
        }

        queue.push(StructSystemDetails::SortEntity(entity, hierarchyIndex));
    }

    // check if we still somebody to delete
    if (queue.empty())
        return;

    // actually delete bastards
    SceneEditor2* sceneEditor = (SceneEditor2*)GetScene();
    sceneEditor->BeginBatch("Remove entities", static_cast<DAVA::uint32>(objects.size()));
    while (!queue.empty())
    {
        StructSystemDetails::SortEntity sortEntity = queue.top();
        queue.pop();
        if (sortEntity.entity->GetNotRemovable() == true)
        {
            continue;
        }

        StructureSystemDelegate* exclusiveRemoveDelegate = nullptr;
        for (StructureSystemDelegate* d : delegates)
        {
            if (d->HasCustomRemovingForEntity(sortEntity.entity) == true)
            {
                DVASSERT(exclusiveRemoveDelegate == nullptr);
                exclusiveRemoveDelegate = d;
            }
        }

        for (StructureSystemDelegate* delegate : delegates)
        {
            delegate->WillRemove(sortEntity.entity);
        }
        if (exclusiveRemoveDelegate != nullptr)
        {
            exclusiveRemoveDelegate->PerformRemoving(sortEntity.entity);
        }
        else
        {
            sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new EntityRemoveCommand(sortEntity.entity)));
        }
        for (StructureSystemDelegate* delegate : delegates)
        {
            delegate->DidRemoved(sortEntity.entity);
        }
    }
    sceneEditor->EndBatch();
}

void StructureSystem::Remove(const SelectableGroup& objects)
{
    SceneEditor2* sceneEditor = (SceneEditor2*)GetScene();
    if ((nullptr == sceneEditor) || objects.IsEmpty())
        return;

    DAVA::Vector<DAVA::Entity*> entitiesToRemove;
    entitiesToRemove.reserve(objects.GetSize());
    for (auto entity : objects.ObjectsOfType<DAVA::Entity>())
    {
        entitiesToRemove.push_back(entity);
    }
    RemoveEntities(entitiesToRemove);
    EmitChanged();
}

void StructureSystem::MoveEmitter(const DAVA::Vector<DAVA::ParticleEmitterInstance*>& emitters, const DAVA::Vector<DAVA::ParticleEffectComponent*>& oldEffects, DAVA::ParticleEffectComponent* newEffect, int dropAfter)
{
    SceneEditor2* sceneEditor = (SceneEditor2*)GetScene();
    if (sceneEditor == nullptr)
        return;

    sceneEditor->BeginBatch("Move particle emitter", static_cast<DAVA::uint32>(emitters.size()));
    for (size_t i = 0; i < emitters.size(); ++i)
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new ParticleEmitterMoveCommand(oldEffects[i], emitters[i], newEffect, dropAfter++)));
    }
    sceneEditor->EndBatch();
    EmitChanged();
}

void StructureSystem::MoveLayer(const DAVA::Vector<DAVA::ParticleLayer*>& layers, const DAVA::Vector<DAVA::ParticleEmitterInstance*>& oldEmitters, DAVA::ParticleEmitterInstance* newEmitter, DAVA::ParticleLayer* newBefore)
{
    SceneEditor2* sceneEditor = (SceneEditor2*)GetScene();
    if (sceneEditor == nullptr)
        return;

    sceneEditor->BeginBatch("Move particle layers", static_cast<DAVA::uint32>(layers.size()));
    for (size_t i = 0; i < layers.size(); ++i)
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new ParticleLayerMoveCommand(oldEmitters[i], layers[i], newEmitter, newBefore)));
    }
    sceneEditor->EndBatch();
    EmitChanged();
}

void StructureSystem::MoveSimplifiedForce(const DAVA::Vector<DAVA::ParticleForceSimplified*>& forces, const DAVA::Vector<DAVA::ParticleLayer*>& oldLayers, DAVA::ParticleLayer* newLayer)
{
    SceneEditor2* sceneEditor = (SceneEditor2*)GetScene();
    if (sceneEditor == nullptr)
        return;

    sceneEditor->BeginBatch("Move particle simplified force", static_cast<DAVA::uint32>(forces.size()));
    for (size_t i = 0; i < forces.size(); ++i)
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new ParticleSimplifiedForceMoveCommand(forces[i], oldLayers[i], newLayer)));
    }
    sceneEditor->EndBatch();
    EmitChanged();
}

void StructureSystem::MoveParticleForce(const DAVA::Vector<DAVA::ParticleForce*>& forces, const DAVA::Vector<DAVA::ParticleLayer*>& oldLayers, DAVA::ParticleLayer* newLayer)
{
    SceneEditor2* sceneEditor = (SceneEditor2*)GetScene();
    if (sceneEditor == nullptr)
        return;

    sceneEditor->BeginBatch("Move particle force", static_cast<DAVA::uint32>(forces.size()));
    for (size_t i = 0; i < forces.size(); ++i)
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new ParticleForceMoveCommand(forces[i], oldLayers[i], newLayer)));
    }
    sceneEditor->EndBatch();
    EmitChanged();
}

SelectableGroup StructureSystem::ReloadEntities(const SelectableGroup& objects, bool saveLightmapSettings)
{
    if (objects.IsEmpty())
        return SelectableGroup();

    DAVA::Set<DAVA::FilePath> refsToReload;

    for (auto entity : objects.ObjectsOfType<DAVA::Entity>())
    {
        DAVA::KeyedArchive* props = GetCustomPropertiesArchieve(entity);
        if (props != nullptr)
        {
            DAVA::FilePath pathToReload(props->GetString(ResourceEditor::EDITOR_REFERENCE_TO_OWNER));
            if (!pathToReload.IsEmpty())
            {
                refsToReload.insert(pathToReload);
            }
        }
    }

    DAVA::Set<DAVA::FilePath>::iterator it = refsToReload.begin();
    InternalMapping groupMapping;
    for (; it != refsToReload.end(); ++it)
    {
        InternalMapping mapping;
        ReloadRefs(*it, mapping, saveLightmapSettings);
        groupMapping.insert(mapping.begin(), mapping.end());
    }

    DVASSERT(dynamic_cast<SceneEditor2*>(GetScene()) != nullptr);
    SceneEditor2* scene = static_cast<SceneEditor2*>(GetScene());

    SelectableGroup result;
    StructSystemDetails::MapSelectableGroup(objects, result, groupMapping, scene->collisionSystem);
    return result;
}

void StructureSystem::ReloadRefs(const DAVA::FilePath& modelPath, InternalMapping& mapping, bool saveLightmapSettings)
{
    if (!modelPath.IsEmpty())
    {
        ReloadInternal(mapping, modelPath, saveLightmapSettings);
    }
}

SelectableGroup StructureSystem::ReloadEntitiesAs(const SelectableGroup& objects, const DAVA::FilePath& newModelPath, bool saveLightmapSettings)
{
    if (objects.IsEmpty())
        return SelectableGroup();

    InternalMapping entitiesToReload;

    for (auto entity : objects.ObjectsOfType<DAVA::Entity>())
    {
        entitiesToReload.emplace(entity, nullptr);
    }

    ReloadInternal(entitiesToReload, newModelPath, saveLightmapSettings);

    DVASSERT(dynamic_cast<SceneEditor2*>(GetScene()) != nullptr);
    SceneEditor2* scene = static_cast<SceneEditor2*>(GetScene());

    SelectableGroup result;
    StructSystemDetails::MapSelectableGroup(objects, result, entitiesToReload, scene->collisionSystem);
    return result;
}

void StructureSystem::ReloadInternal(InternalMapping& mapping, const DAVA::FilePath& newModelPath, bool saveLightmapSettings)
{
    SceneEditor2* sceneEditor = (SceneEditor2*)GetScene();
    if (NULL != sceneEditor)
    {
        // also we should reload all entities, that already has reference to the same newModelPath
        SearchEntityByRef(GetScene(), newModelPath, [&mapping](DAVA::Entity* item) {
            mapping.emplace(item, nullptr);
        });

        if (mapping.size() > 0)
        {
            // try to load new model
            DAVA::Entity* loadedEntity = LoadInternal(newModelPath, true);

            if (NULL != loadedEntity)
            {
                InternalMapping::iterator it = mapping.begin();
                InternalMapping::iterator end = mapping.end();

                sceneEditor->BeginBatch("Reload model", static_cast<DAVA::uint32>(mapping.size()));

                for (; it != end; ++it)
                {
                    DAVA::ScopedPtr<DAVA::Entity> newEntityInstance(loadedEntity->Clone());
                    DAVA::Entity* origEntity = it->first;

                    if ((origEntity != nullptr) && (newEntityInstance.get() != nullptr) && (origEntity->GetParent() != nullptr))
                    {
                        DAVA::Entity* before = origEntity->GetParent()->GetNextChild(origEntity);

                        newEntityInstance->SetLocalTransform(origEntity->GetLocalTransform());
                        newEntityInstance->SetID(origEntity->GetID());
                        newEntityInstance->SetSceneID(origEntity->GetSceneID());
                        newEntityInstance->SetNotRemovable(origEntity->GetNotRemovable());
                        it->second = newEntityInstance;

                        if (saveLightmapSettings)
                        {
                            CopyLightmapSettings(origEntity, newEntityInstance);
                        }

                        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new EntityParentChangeCommand(newEntityInstance, origEntity->GetParent(), before)));
                        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new EntityRemoveCommand(origEntity)));
                    }
                }

                sceneEditor->EndBatch();
                loadedEntity->Release();
            }
        }
    }
}

void StructureSystem::Add(const DAVA::FilePath& newModelPath, const DAVA::Vector3 pos)
{
    SceneEditor2* sceneEditor = (SceneEditor2*)GetScene();
    if (nullptr != sceneEditor)
    {
        DAVA::ScopedPtr<DAVA::Entity> loadedEntity(Load(newModelPath));
        if (static_cast<DAVA::Entity*>(loadedEntity) != nullptr)
        {
            DAVA::Vector3 entityPos = pos;
            if (entityPos.IsZero() && FindLandscape(loadedEntity) == nullptr)
            {
                SceneCameraSystem* cameraSystem = sceneEditor->cameraSystem;

                DAVA::Vector3 camDirection = cameraSystem->GetCameraDirection();
                DAVA::Vector3 camPosition = cameraSystem->GetCameraPosition();

                DAVA::AABBox3 commonBBox = loadedEntity->GetWTMaximumBoundingBoxSlow();
                DAVA::float32 bboxSize = 5.0f;

                if (!commonBBox.IsEmpty())
                {
                    bboxSize += (commonBBox.max - commonBBox.min).Length();
                }

                camDirection.Normalize();

                entityPos = camPosition + camDirection * bboxSize;
            }

            DAVA::Matrix4 transform = loadedEntity->GetLocalTransform();
            transform.SetTranslationVector(entityPos);
            loadedEntity->SetLocalTransform(transform);

            sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new EntityAddCommand(loadedEntity, sceneEditor)));

            // TODO: move this code to some another place (into command itself or into ProcessCommand function)
            //
            // Перенести в Load и завалидейтить только подгруженную Entity
            // -->
            SceneValidator validator;
            ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
            if (data)
            {
                validator.SetPathForChecking(data->GetProjectPath());
            }
            validator.ValidateScene(sceneEditor, sceneEditor->GetScenePath());
            // <--

            EmitChanged();
        }
    }
}

void StructureSystem::EmitChanged()
{
    // mark that structure was changed. real signal will be emited on next update() call
    // this should done be to increase performance - on Change emit on multiple scene structure operations
    structureChanged = true;
}

void StructureSystem::AddDelegate(StructureSystemDelegate* delegate)
{
    delegates.push_back(delegate);
}

void StructureSystem::RemoveDelegate(StructureSystemDelegate* delegate)
{
    delegates.remove(delegate);
}

void StructureSystem::Process(DAVA::float32 timeElapsed)
{
    if (structureChanged)
    {
        SceneSignals::Instance()->EmitStructureChanged((SceneEditor2*)GetScene(), nullptr);
        structureChanged = false;
    }
}

void StructureSystem::ProcessCommand(const RECommandNotificationObject& commandNotification)
{
    if (commandNotification.MatchCommandIDs({ CMDID_PARTICLE_LAYER_REMOVE, CMDID_PARTICLE_LAYER_MOVE, CMDID_PARTICLE_FORCE_REMOVE, CMDID_PARTICLE_SIMPLIFIED_FORCE_MOVE, CMDID_PARTICLE_FORCE_MOVE }))
    {
        EmitChanged();
    }
}

void StructureSystem::AddEntity(DAVA::Entity* entity)
{
    EmitChanged();
}

void StructureSystem::RemoveEntity(DAVA::Entity* entity)
{
    EmitChanged();
}

void StructureSystem::PrepareForRemove()
{
    EmitChanged();
}

void StructureSystem::CheckAndMarkSolid(DAVA::Entity* entity)
{
    if (nullptr != entity)
    {
        DAVA::int32 numChildren = entity->GetChildrenCount();
        for (DAVA::int32 i = 0; i < numChildren; ++i)
        {
            CheckAndMarkSolid(entity->GetChild(i));
        }
        entity->SetSolid(numChildren > 0);
    }
}

DAVA::Entity* StructureSystem::Load(const DAVA::FilePath& sc2path)
{
    return LoadInternal(sc2path, false);
}

DAVA::Entity* StructureSystem::LoadInternal(const DAVA::FilePath& sc2path, bool clearCache)
{
    DAVA::Entity* loadedEntity = nullptr;

    SceneEditor2* sceneEditor = (SceneEditor2*)GetScene();
    if (nullptr != sceneEditor && sc2path.IsEqualToExtension(".sc2") && DAVA::FileSystem::Instance()->Exists(sc2path))
    {
        if (clearCache)
        {
            // if there is already entity for such file, we should release it
            // to be sure that latest version will be loaded
            sceneEditor->cache.Clear(sc2path);
        }

        loadedEntity = sceneEditor->cache.GetClone(sc2path);
        if (nullptr != loadedEntity)
        {
            // this is for backward compatibility.
            // sceneFileV2 will remove empty nodes only
            // if there is parent for such nodes.
            {
                DAVA::ScopedPtr<DAVA::SceneFileV2> tmpSceneFile(new DAVA::SceneFileV2());
                DAVA::ScopedPtr<DAVA::Entity> tmpParent(new DAVA::Entity());
                DAVA::Entity* tmpEntity = loadedEntity;

                tmpParent->AddNode(tmpEntity);
                tmpSceneFile->RemoveEmptyHierarchy(tmpEntity);

                loadedEntity = SafeRetain(tmpParent->GetChild(0));

                DAVA::SafeRelease(tmpEntity);
            }

            DAVA::KeyedArchive* props = GetOrCreateCustomProperties(loadedEntity)->GetArchive();
            props->SetString(ResourceEditor::EDITOR_REFERENCE_TO_OWNER, sc2path.GetAbsolutePathname());

            CheckAndMarkSolid(loadedEntity);

            SceneValidator::ExtractEmptyRenderObjects(loadedEntity);
        }
    }
    else
    {
        DAVA::Logger::Error("Wrong extension or no such file: %s", sc2path.GetAbsolutePathname().c_str());
    }

    return loadedEntity;
}

void StructureSystem::CopyLightmapSettings(DAVA::NMaterial* fromState, DAVA::NMaterial* toState) const
{
    if (fromState->HasLocalTexture(DAVA::NMaterialTextureName::TEXTURE_LIGHTMAP))
    {
        DAVA::Texture* lightmap = fromState->GetLocalTexture(DAVA::NMaterialTextureName::TEXTURE_LIGHTMAP);
        if (toState->HasLocalTexture(DAVA::NMaterialTextureName::TEXTURE_LIGHTMAP))
            toState->SetTexture(DAVA::NMaterialTextureName::TEXTURE_LIGHTMAP, lightmap);
        else
            toState->AddTexture(DAVA::NMaterialTextureName::TEXTURE_LIGHTMAP, lightmap);
    }

    if (fromState->HasLocalProperty(DAVA::NMaterialParamName::PARAM_UV_SCALE))
    {
        const float* data = fromState->GetLocalPropValue(DAVA::NMaterialParamName::PARAM_UV_SCALE);
        if (toState->HasLocalProperty(DAVA::NMaterialParamName::PARAM_UV_SCALE))
            toState->SetPropertyValue(DAVA::NMaterialParamName::PARAM_UV_SCALE, data);
        else
            toState->AddProperty(DAVA::NMaterialParamName::PARAM_UV_SCALE, data, rhi::ShaderProp::TYPE_FLOAT2);
    }

    if (fromState->HasLocalProperty(DAVA::NMaterialParamName::PARAM_UV_OFFSET))
    {
        const float* data = fromState->GetLocalPropValue(DAVA::NMaterialParamName::PARAM_UV_OFFSET);
        if (toState->HasLocalProperty(DAVA::NMaterialParamName::PARAM_UV_OFFSET))
            toState->SetPropertyValue(DAVA::NMaterialParamName::PARAM_UV_OFFSET, data);
        else
            toState->AddProperty(DAVA::NMaterialParamName::PARAM_UV_OFFSET, data, rhi::ShaderProp::TYPE_FLOAT2);
    }
}

struct BatchInfo
{
    BatchInfo()
        : switchIndex(-1)
        , lodIndex(-1)
        , batch(NULL)
    {
    }

    DAVA::int32 switchIndex;
    DAVA::int32 lodIndex;

    DAVA::RenderBatch* batch;
};

struct SortBatches
{
    bool operator()(const BatchInfo& b1, const BatchInfo& b2)
    {
        if (b1.switchIndex == b2.switchIndex)
        {
            return b1.lodIndex < b2.lodIndex;
        }

        return b1.switchIndex < b2.switchIndex;
    }
};

void CreateBatchesInfo(DAVA::RenderObject* object, DAVA::Vector<BatchInfo>& batches)
{
    if (!object)
        return;

    DAVA::uint32 batchesCount = object->GetRenderBatchCount();
    for (DAVA::uint32 i = 0; i < batchesCount; ++i)
    {
        BatchInfo info;
        info.batch = object->GetRenderBatch(i, info.lodIndex, info.switchIndex);
        batches.push_back(info);
    }

    std::sort(batches.begin(), batches.end(), SortBatches());
}

bool StructureSystem::CopyLightmapSettings(DAVA::Entity* fromEntity, DAVA::Entity* toEntity) const
{
    DAVA::Vector<DAVA::RenderObject*> fromMeshes;
    FindMeshesRecursive(fromEntity, fromMeshes);

    DAVA::Vector<DAVA::RenderObject*> toMeshes;
    FindMeshesRecursive(toEntity, toMeshes);

    if (fromMeshes.size() == toMeshes.size())
    {
        DAVA::uint32 meshCount = (DAVA::uint32)fromMeshes.size();
        for (DAVA::uint32 m = 0; m < meshCount; ++m)
        {
            DAVA::Vector<BatchInfo> fromBatches;
            CreateBatchesInfo(fromMeshes[m], fromBatches);

            DAVA::Vector<BatchInfo> toBatches;
            CreateBatchesInfo(toMeshes[m], toBatches);

            DAVA::uint32 rbFromCount = fromMeshes[m]->GetRenderBatchCount();
            DAVA::uint32 rbToCount = toMeshes[m]->GetRenderBatchCount();

            for (DAVA::uint32 from = 0, to = 0; from < rbFromCount && to < rbToCount;)
            {
                BatchInfo& fromBatch = fromBatches[from];
                BatchInfo& toBatch = toBatches[to];

                if (fromBatch.switchIndex == toBatch.switchIndex)
                {
                    if (fromBatch.lodIndex <= toBatch.lodIndex)
                    {
                        for (DAVA::uint32 usedToIndex = to; usedToIndex < rbToCount; ++usedToIndex)
                        {
                            BatchInfo& usedToBatch = toBatches[usedToIndex];

                            if ((fromBatch.switchIndex != usedToBatch.switchIndex))
                                break;

                            DAVA::PolygonGroup* fromPG = fromBatch.batch->GetPolygonGroup();
                            DAVA::PolygonGroup* toPG = usedToBatch.batch->GetPolygonGroup();

                            DAVA::uint32 fromSize = fromPG->GetVertexCount() * fromPG->vertexStride;
                            DAVA::uint32 toSize = toPG->GetVertexCount() * toPG->vertexStride;
                            if ((fromSize == toSize) && (0 == Memcmp(fromPG->meshData, toPG->meshData, fromSize)))
                            {
                                CopyLightmapSettings(fromBatch.batch->GetMaterial(), usedToBatch.batch->GetMaterial());
                            }
                        }

                        ++from;
                    }
                    else if (fromBatch.lodIndex < toBatch.lodIndex)
                    {
                        ++from;
                    }
                    else
                    {
                        ++to;
                    }
                }
                else if (fromBatch.switchIndex < toBatch.switchIndex)
                {
                    ++from;
                }
                else
                {
                    ++to;
                }
            }
        }

        return true;
    }

    return false;
}

void StructureSystem::FindMeshesRecursive(DAVA::Entity* entity, DAVA::Vector<DAVA::RenderObject*>& objects) const
{
    DAVA::RenderObject* ro = GetRenderObject(entity);
    if (ro && ro->GetType() == DAVA::RenderObject::TYPE_MESH)
    {
        objects.push_back(ro);
    }

    DAVA::int32 count = entity->GetChildrenCount();
    for (DAVA::int32 i = 0; i < count; ++i)
    {
        FindMeshesRecursive(entity->GetChild(i), objects);
    }
}

void StructureSystem::SearchEntityByRef(DAVA::Entity* parent, const DAVA::FilePath& refToOwner, const DAVA::Function<void(DAVA::Entity*)>& callback)
{
    DVASSERT(callback);
    if (NULL != parent)
    {
        for (int i = 0; i < parent->GetChildrenCount(); ++i)
        {
            DAVA::Entity* entity = parent->GetChild(i);
            DAVA::KeyedArchive* arch = GetCustomPropertiesArchieve(entity);

            if (arch)
            {
                // if this entity has searched reference - add it to the set
                if (DAVA::FilePath(arch->GetString(ResourceEditor::EDITOR_REFERENCE_TO_OWNER, "")) == refToOwner)
                {
                    callback(entity);
                    continue;
                }
            }

            // else continue searching in child entities
            SearchEntityByRef(entity, refToOwner, callback);
        }
    }
}
