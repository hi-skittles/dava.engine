#include "SaveEntityAsAction.h"

#include <REPlatform/DataNodes/SelectableGroup.h>
#include <REPlatform/Global/StringConstants.h>
#include "Math/Transform.h"

#include <Render/Highlevel/RenderBatch.h>
#include <Render/Highlevel/RenderObject.h>
#include <Render/Material/NMaterial.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/TransformComponent.h>
#include <Scene3D/Scene.h>
#include <Scene3D/SceneFileV2.h>
#include <Scene3D/Systems/StaticOcclusionSystem.h>

class ElegantSceneGuard final
{
public:
    ElegantSceneGuard(DAVA::Scene* scene_)
        : scene(scene_)
    {
        using namespace DAVA;
        if (scene)
        {
            Set<DataNode*> nodes;
            scene->GetDataNodes(nodes);
            for (auto& node : nodes)
            {
                dataNodeIDs[node] = node->GetNodeID();
            }

            globalMaterial = SafeRetain(scene->GetGlobalMaterial());
            scene->SetGlobalMaterial(nullptr);
        }
    }

    ~ElegantSceneGuard()
    {
        using namespace DAVA;
        for (auto& id : dataNodeIDs)
        {
            id.first->SetNodeID(id.second);
        }

        if (scene)
        {
            scene->SetGlobalMaterial(globalMaterial);
            scene = nullptr;
        }

        SafeRelease(globalMaterial);
    }

private:
    DAVA::Scene* scene = nullptr;
    DAVA::NMaterial* globalMaterial = nullptr;
    DAVA::Map<DAVA::DataNode*, DAVA::uint64> dataNodeIDs;
};

SaveEntityAsAction::SaveEntityAsAction(const DAVA::SelectableGroup* entities_, const DAVA::FilePath& path_)
    : entities(entities_)
    , sc2Path(path_)
{
}

void SaveEntityAsAction::Run()
{
    using namespace DAVA;
    uint32 count = static_cast<uint32>(entities->GetSize());
    if (!sc2Path.IsEmpty() && sc2Path.IsEqualToExtension(".sc2") && (nullptr != entities) && (count > 0))
    {
        const auto RemoveReferenceToOwner = [](Entity* entity) {
            KeyedArchive* props = GetCustomPropertiesArchieve(entity);
            if (nullptr != props)
            {
                props->DeleteKey(ResourceEditor::EDITOR_REFERENCE_TO_OWNER);
            }
        };

        auto firstEntity = entities->GetFirst().AsEntity();
        DVASSERT(firstEntity != nullptr);
        ElegantSceneGuard guard(firstEntity->GetScene());

        ScopedPtr<Scene> scene(new Scene());
        ScopedPtr<Entity> container(nullptr);

        if (count == 1) // saving of single object
        {
            container.reset(firstEntity->Clone());
            RemoveReferenceToOwner(container);

            TransformComponent* tc = container->GetComponent<TransformComponent>();
            tc->SetLocalTransform(Transform());
        }
        else // saving of group of objects
        {
            container.reset(new Entity());

            const Vector3 oldZero = entities->GetCommonWorldSpaceTranslationVector();
            for (auto entity : entities->ObjectsOfType<DAVA::Entity>())
            {
                ScopedPtr<Entity> clone(entity->Clone());
                TransformComponent* tc = clone->GetComponent<TransformComponent>();

                const Vector3 offset = tc->GetLocalTransform().GetTranslation() - oldZero;
                tc->SetLocalTranslation(offset);

                container->AddNode(clone);
                RemoveReferenceToOwner(clone);
            }

            container->SetName(sc2Path.GetFilename().c_str());
        }
        DVASSERT(container);

        scene->AddNode(container); //1. Added new items in zero position with identity matrix
        scene->staticOcclusionSystem->InvalidateOcclusion(); //2. invalidate static occlusion indeces
        RemoveLightmapsRecursive(container); //3. Reset lightmaps

        scene->SaveScene(sc2Path);
    }
}

void SaveEntityAsAction::RemoveLightmapsRecursive(DAVA::Entity* entity) const
{
    using namespace DAVA;
    RenderObject* renderObject = GetRenderObject(entity);
    if (nullptr != renderObject)
    {
        const uint32 batchCount = renderObject->GetRenderBatchCount();
        for (uint32 b = 0; b < batchCount; ++b)
        {
            NMaterial* material = renderObject->GetRenderBatch(b)->GetMaterial();
            if ((nullptr != material) && material->HasLocalTexture(NMaterialTextureName::TEXTURE_LIGHTMAP))
            {
                material->RemoveTexture(NMaterialTextureName::TEXTURE_LIGHTMAP);
            }
        }
    }

    const int32 count = entity->GetChildrenCount();
    for (int32 ch = 0; ch < count; ++ch)
    {
        RemoveLightmapsRecursive(entity->GetChild(ch));
    }
}
