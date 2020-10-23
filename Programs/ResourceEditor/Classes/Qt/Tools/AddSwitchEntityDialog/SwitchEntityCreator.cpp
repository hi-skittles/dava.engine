#include "Classes/Qt/Tools/AddSwitchEntityDialog/SwitchEntityCreator.h"

#include <REPlatform/Global/StringConstants.h>

#include <FileSystem/KeyedArchive.h>
#include <Logger/Logger.h>
#include <Math/Matrix4.h>
#include <Render/3D/PolygonGroup.h>
#include <Render/Highlevel/Mesh.h>
#include <Render/Highlevel/RenderBatch.h>
#include <Render/Highlevel/RenderObject.h>
#include <Render/Highlevel/SkinnedMesh.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/CustomPropertiesComponent.h>
#include <Scene3D/Components/MotionComponent.h>
#include <Scene3D/Components/RenderComponent.h>
#include <Scene3D/Components/SkeletonComponent.h>
#include <Scene3D/Components/SwitchComponent.h>
#include <Scene3D/Components/TransformComponent.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Lod/LodComponent.h>

DAVA::RefPtr<DAVA::Entity> SwitchEntityCreator::CreateSwitchEntity(const DAVA::Vector<DAVA::Entity*>& sourceEntities)
{
    using namespace DAVA;

    RefPtr<Entity> switchEntity(new Entity);

    switchEntity->AddComponent(new DAVA::SwitchComponent());
    switchEntity->SetName(DAVA::FastName(DAVA::ResourceEditor::SWITCH_NODE_NAME));
    switchEntity->SetSolid(false);

    size_t switchCount = sourceEntities.size();
    DVASSERT(switchCount <= MAX_SWITCH_COUNT);

    clonedEntities.reserve(switchCount);
    bool singleMode = true;
    for (DAVA::uint32 switchIndex = 0; switchIndex < switchCount; ++switchIndex)
    {
        clonedEntities.push_back(sourceEntities[switchIndex]->Clone());

        FindRenderObjectsRecursive(clonedEntities[switchIndex], renderPairs[switchIndex]);
        if (renderPairs[switchIndex].size() > 1)
        {
            singleMode = false;
        }

        DAVA::KeyedArchive* childProps = GetCustomPropertiesArchieve(clonedEntities[switchIndex]);
        if (childProps != nullptr && childProps->IsKeyExists("CollisionType"))
        {
            DAVA::KeyedArchive* customProperties = GetOrCreateCustomProperties(switchEntity.Get())->GetArchive();
            if (0 == switchIndex)
            {
                customProperties->SetInt32("CollisionType", childProps->GetInt32("CollisionType", 0));
            }
            else
            {
                customProperties->SetInt32("CollisionTypeCrashed", childProps->GetInt32("CollisionType", 0));
            }
        }
    }

    if (singleMode)
    {
        CreateSingleObjectData(switchEntity.Get());
    }
    else
    {
        CreateMultipleObjectsData();
    }

    for (DAVA::uint32 switchIndex = 0; switchIndex < switchCount; ++switchIndex)
    {
        SafeRelease(clonedEntities[switchIndex]);
    }
    clonedEntities.clear();

    for (DAVA::Entity* realChild : realChildren)
    {
        switchEntity->AddNode(realChild);
        realChild->Release();
    }
    realChildren.clear();

    return switchEntity;
}

void SwitchEntityCreator::FindRenderObjectsRecursive(DAVA::Entity* fromEntity, DAVA::Vector<RenderPair>& entityAndObjectPairs)
{
    DAVA::RenderObject* ro = GetRenderObject(fromEntity);
    if (ro != nullptr && (ro->GetType() == DAVA::RenderObject::TYPE_MESH || ro->GetType() == DAVA::RenderObject::TYPE_SKINNED_MESH))
    {
        if (ro->GetType() == DAVA::RenderObject::TYPE_SKINNED_MESH)
        {
            hasSkinnedMesh = true;
        }
        entityAndObjectPairs.emplace_back(fromEntity, ro);
    }

    DAVA::int32 size = fromEntity->GetChildrenCount();
    for (DAVA::int32 i = 0; i < size; ++i)
    {
        DAVA::Entity* child = fromEntity->GetChild(i);
        FindRenderObjectsRecursive(child, entityAndObjectPairs);
    }
}

bool SwitchEntityCreator::HasMeshRenderObjectsRecursive(DAVA::Entity* fromEntity)
{
    DAVA::RenderObject* ro = GetRenderObject(fromEntity);
    if (ro && ro->GetType() == DAVA::RenderObject::TYPE_MESH)
    {
        return true;
    }

    DAVA::int32 size = fromEntity->GetChildrenCount();
    for (DAVA::int32 i = 0; i < size; ++i)
    {
        DAVA::Entity* child = fromEntity->GetChild(i);
        if (HasMeshRenderObjectsRecursive(child))
            return true;
    }

    return false;
}

size_t SwitchEntityCreator::GetRenderObjectsCountRecursive(DAVA::Entity* entity, DAVA::RenderObject::eType objectType)
{
    size_t count = 0u;

    DAVA::RenderObject* ro = GetRenderObject(entity);
    if (ro != nullptr && ro->GetType() == objectType)
    {
        ++count;
    }

    DAVA::int32 size = entity->GetChildrenCount();
    for (DAVA::int32 i = 0; i < size; ++i)
    {
        count += GetRenderObjectsCountRecursive(entity->GetChild(i), objectType);
    }

    return count;
}

bool SwitchEntityCreator::HasSwitchComponentsRecursive(DAVA::Entity* fromEntity)
{
    if (GetSwitchComponent(fromEntity))
    {
        return true;
    }

    DAVA::int32 size = fromEntity->GetChildrenCount();
    for (DAVA::int32 i = 0; i < size; ++i)
    {
        if (HasSwitchComponentsRecursive(fromEntity->GetChild(i)))
            return true;
    }

    return false;
}

void SwitchEntityCreator::CreateSingleObjectData(DAVA::Entity* switchEntity)
{
    DAVA::RenderObject* singleRenderObject = nullptr;
    DAVA::SkinnedMesh* singleSkinnedMeshObject = nullptr;
    if (hasSkinnedMesh)
    {
        singleSkinnedMeshObject = new DAVA::SkinnedMesh;
        singleRenderObject = singleSkinnedMeshObject;
    }
    else
    {
        singleRenderObject = new DAVA::Mesh;
    }

    singleRenderObject->SetAABBox(DAVA::AABBox3(DAVA::Vector3(0, 0, 0), DAVA::Vector3(0, 0, 0)));
    switchEntity->AddComponent(new DAVA::RenderComponent(singleRenderObject));

    DAVA::Set<DAVA::PolygonGroup*> bakedPolygonGroups;
    size_t switchCount = clonedEntities.size();
    for (size_t switchIndex = 0; switchIndex < switchCount; ++switchIndex)
    {
        DAVA::Entity* sourceEntity = clonedEntities[switchIndex];

        DAVA::RenderObject* sourceRenderObject = renderPairs[switchIndex][0].second;
        if (sourceRenderObject)
        {
            DAVA::TransformComponent* sourceTransform = GetTransformComponent(sourceEntity);
            if (sourceTransform->GetLocalTransform() != DAVA::Matrix4::IDENTITY)
            {
                DAVA::PolygonGroup* pg = sourceRenderObject->GetRenderBatchCount() > 0 ? sourceRenderObject->GetRenderBatch(0)->GetPolygonGroup() : nullptr;
                if (pg != nullptr && bakedPolygonGroups.end() == bakedPolygonGroups.find(pg))
                {
                    sourceRenderObject->BakeGeometry(sourceTransform->GetLocalMatrix());
                    bakedPolygonGroups.insert(pg);
                }

                if (sourceRenderObject->GetType() == DAVA::RenderObject::TYPE_SKINNED_MESH)
                {
                    DAVA::Logger::Error("Not-identity transform is not supported for skinned mesh objects");
                }
            }

            DAVA::uint32 renderBatchesRemaining = sourceRenderObject->GetRenderBatchCount();
            while (renderBatchesRemaining > 0)
            {
                DAVA::int32 batchLodIndex = -1;
                DAVA::int32 batchSwitchIndex = -1;
                DAVA::RefPtr<DAVA::RenderBatch> sourceRenderBatch = DAVA::RefPtr<DAVA::RenderBatch>::ConstructWithRetain(sourceRenderObject->GetRenderBatch(0, batchLodIndex, batchSwitchIndex));
                sourceRenderObject->RemoveRenderBatch(sourceRenderBatch.Get());
                singleRenderObject->AddRenderBatch(sourceRenderBatch.Get(), batchLodIndex, static_cast<DAVA::int32>(switchIndex));
                --renderBatchesRemaining;

                if (hasSkinnedMesh && sourceRenderObject->GetType() == DAVA::RenderObject::TYPE_SKINNED_MESH)
                {
                    DAVA::SkinnedMesh* sourceSkinnedMesh = dynamic_cast<DAVA::SkinnedMesh*>(sourceRenderObject);
                    auto jointTargets = sourceSkinnedMesh->GetJointTargets(sourceRenderBatch.Get());
                    singleSkinnedMeshObject->SetJointTargets(sourceRenderBatch.Get(), jointTargets);
                }
            }
        }

        auto copyComponent = [&](const DAVA::Type* componentType)
        {
            DAVA::Component* c = sourceEntity->GetComponent(componentType, 0);
            if (c != nullptr && switchEntity->GetComponent(componentType, 0) == nullptr)
            {
                switchEntity->AddComponent(c->Clone(switchEntity));
            }
        };

        copyComponent(DAVA::Type::Instance<DAVA::LodComponent>());
        if (hasSkinnedMesh)
        {
            copyComponent(DAVA::Type::Instance<DAVA::SkeletonComponent>());
            copyComponent(DAVA::Type::Instance<DAVA::MotionComponent>());
        }

        renderPairs[switchIndex][0].first->RemoveComponent(DAVA::Type::Instance<DAVA::RenderComponent>());
        renderPairs[switchIndex][0].first->RemoveComponent(DAVA::Type::Instance<DAVA::LodComponent>());

        DAVA::uint32 childrenRemaining = sourceEntity->GetChildrenCount();
        while (childrenRemaining)
        {
            DAVA::Entity* child = sourceEntity->GetChild(0);
            child->Retain();

            sourceEntity->RemoveNode(child);
            realChildren.push_back(child);

            --childrenRemaining;
        }
    }
}

void SwitchEntityCreator::CreateMultipleObjectsData()
{
    DAVA::uint32 count = (DAVA::uint32)clonedEntities.size();
    for (DAVA::uint32 i = 0; i < count; ++i)
    {
        DAVA::uint32 pairsCount = (DAVA::uint32)renderPairs[i].size();
        for (DAVA::uint32 p = 0; p < pairsCount; ++p)
        {
            DAVA::RenderObject* sourceRenderObject = renderPairs[i][p].second;
            DAVA::uint32 batchCount = sourceRenderObject->GetRenderBatchCount();
            for (DAVA::int32 b = (DAVA::int32)(batchCount - 1); b >= 0; --b)
            {
                DAVA::int32 lodIndex = -1, switchIndex = -1;
                DAVA::RenderBatch* batch = sourceRenderObject->GetRenderBatch(b, lodIndex, switchIndex);
                batch->Retain();

                sourceRenderObject->RemoveRenderBatch(b);
                sourceRenderObject->AddRenderBatch(batch, lodIndex, i);

                batch->Release();
            }
        }

        realChildren.push_back(clonedEntities[i]);
        clonedEntities[i] = NULL;
    }
}
