#include "Classes/CommandLine/Private/ResourceDependency/SceneDependency.h"
#include "Classes/CommandLine/Private/ResourceDependency/ResourceDependencyConstants.h"

#include <Base/ScopedPtr.h>
#include <Debug/DVAssert.h>
#include <Render/Highlevel/RenderObject.h>
#include <Render/Highlevel/RenderBatch.h>
#include <Render/Highlevel/Landscape.h>
#include <Render/Highlevel/Vegetation/VegetationRenderObject.h>
#include <Render/Material/NMaterial.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Scene.h>
#include <Scene3D/SceneFileV2.h>
#include <Scene3D/Components/MotionComponent.h>
#include <Scene3D/Components/ParticleEffectComponent.h>
#include <Scene3D/Components/RenderComponent.h>
#include <Scene3D/Components/SlotComponent.h>
#include <Scene3D/Systems/SlotSystem.h>

namespace SceneDependencyDetails
{
void EnumerateAnimations(DAVA::Entity* entity, DAVA::Set<DAVA::FilePath>& dependencies)
{
    using namespace DAVA;

    for (uint32 i = 0; i < entity->GetComponentCount<MotionComponent>(); ++i)
    {
        MotionComponent* comp = entity->GetComponent<MotionComponent>(i);
        Vector<FilePath> deps = comp->GetDependencies();
        dependencies.insert(deps.begin(), deps.end());
    }
}

void EnumerateEffects(DAVA::Entity* entity, DAVA::Set<DAVA::FilePath>& dependencies)
{
    using namespace DAVA;

    std::function<void(ParticleEmitterInstance * instance, Set<FilePath> & dependencies)> enumerateEmitter = [&](ParticleEmitterInstance* instance, Set<FilePath>& dependencies)
    {
        ParticleEmitter* emitter = instance->GetEmitter();
        dependencies.insert(emitter->configPath);

        for (ParticleLayer* layer : emitter->layers)
        {
            if (layer->type == ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
            {
                enumerateEmitter(layer->innerEmitter, dependencies);
            }
            else
            {
                //TODO: process sprites in case we will need it
            }
        }
    };

    for (uint32 i = 0; i < entity->GetComponentCount<ParticleEffectComponent>(); ++i)
    {
        ParticleEffectComponent* comp = entity->GetComponent<ParticleEffectComponent>(i);
        const int32 emittersCount = comp->GetEmittersCount();
        for (int32 em = 0; em < emittersCount; ++em)
        {
            enumerateEmitter(comp->GetEmitterInstance(em), dependencies);
        }
    }
}

void EnumerateRenderObjects(DAVA::Entity* entity, DAVA::Set<DAVA::FilePath>& dependencies)
{
    using namespace DAVA;

    auto enumerateMaterial = [](NMaterial* material, Set<FilePath>& dependencies)
    {
        //Enumerate textures from materials
        Set<MaterialTextureInfo*> materialTextures;
        while (nullptr != material)
        {
            material->CollectLocalTextures(materialTextures);
            material = material->GetParent();
        }

        // enumerate descriptor pathnames
        for (const MaterialTextureInfo* matTex : materialTextures)
        {
            dependencies.insert(matTex->path);
        }
    };

    for (uint32 i = 0; i < entity->GetComponentCount<RenderComponent>(); ++i)
    {
        RenderComponent* comp = entity->GetComponent<RenderComponent>(i);
        RenderObject* ro = comp->GetRenderObject();
        DVASSERT(ro != nullptr);

        switch (ro->GetType())
        {
        case RenderObject::TYPE_LANDSCAPE:
        {
            Landscape* landscape = static_cast<Landscape*>(ro);
            dependencies.insert(landscape->GetHeightmapPathname());

            enumerateMaterial(landscape->GetMaterial(), dependencies);
            break;
        }
        case RenderObject::TYPE_VEGETATION:
        {
            VegetationRenderObject* vegetation = static_cast<VegetationRenderObject*>(ro);
            dependencies.insert(vegetation->GetHeightmapPath());
            dependencies.insert(vegetation->GetCustomGeometryPath());
            dependencies.insert(vegetation->GetLightmapPath());

            Set<DataNode*> dataNodes;
            vegetation->GetDataNodes(dataNodes);
            for (DataNode* node : dataNodes)
            {
                NMaterial* material = dynamic_cast<NMaterial*>(node);
                if (material != nullptr)
                {
                    enumerateMaterial(material, dependencies);
                }
            }
            break;
        }

        default:
            break;
        }

        uint32 count = ro->GetRenderBatchCount();
        for (uint32 rb = 0; rb < count; ++rb)
        {
            RenderBatch* batch = ro->GetRenderBatch(rb);
            enumerateMaterial(batch->GetMaterial(), dependencies);
        }
    }
}

void EnumerateSlots(DAVA::Entity* entity, DAVA::Set<DAVA::FilePath>& dependencies)
{
    using namespace DAVA;

    for (uint32 i = 0; i < entity->GetComponentCount<SlotComponent>(); ++i)
    {
        SlotComponent* comp = entity->GetComponent<SlotComponent>(i);
        dependencies.insert(comp->GetConfigFilePath());
    }
}

void EnumerateEntity(DAVA::Entity* entity, DAVA::Set<DAVA::FilePath>& dependencies)
{
    using namespace DAVA;

    { //recursion
        uint32 count = entity->GetChildrenCount();
        for (uint32 ch = 0; ch < count; ++ch)
        {
            EnumerateEntity(entity->GetChild(ch), dependencies);
        }
    }

    EnumerateAnimations(entity, dependencies);
    EnumerateEffects(entity, dependencies);
    EnumerateRenderObjects(entity, dependencies);
    EnumerateSlots(entity, dependencies);
}
}

bool SceneDependency::GetDependencies(const DAVA::FilePath& scenePath, DAVA::Set<DAVA::FilePath>& dependencies, DAVA::int32 requestedType)
{
    using namespace DAVA;

    if (requestedType == static_cast<eDependencyType>(eDependencyType::CONVERT))
    { // right now we don't have dependencies for convertion of scene
        return true;
    }
    else if (requestedType == static_cast<eDependencyType>(eDependencyType::DOWNLOAD))
    {
        ScopedPtr<Scene> scene(new Scene());
        if (SceneFileV2::ERROR_NO_ERROR == scene->LoadScene(scenePath))
        {
            SceneDependencyDetails::EnumerateEntity(scene, dependencies);
            return true;
        }
    }

    return false;
};
