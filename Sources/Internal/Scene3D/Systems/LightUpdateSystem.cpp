#include "Scene3D/Systems/LightUpdateSystem.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/LightComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/SingleComponents/TransformSingleComponent.h"
#include "Render/Highlevel/Frustum.h"
#include "Render/Highlevel/Camera.h"
#include "Render/Highlevel/Landscape.h"

#include "Render/Highlevel/RenderLayer.h"
#include "Render/Highlevel/RenderPass.h"
#include "Render/Highlevel/RenderBatch.h"
#include "Render/Highlevel/RenderSystem.h"
#include "Scene3D/Scene.h"
#include "Time/SystemTimer.h"

namespace DAVA
{
LightUpdateSystem::LightUpdateSystem(Scene* scene)
    : SceneSystem(scene)
{
}

void LightUpdateSystem::Process(float32 timeElapsed)
{
    TransformSingleComponent* tsc = GetScene()->transformSingleComponent;

    for (auto& pair : tsc->worldTransformChanged.map)
    {
        if (pair.first->GetComponentsCount(Type::Instance<LightComponent>()) > 0)
        {
            for (Entity* entity : pair.second)
            {
                RecalcLight(entity);
            }
        }
    }
}

void LightUpdateSystem::RecalcLight(Entity* entity)
{
    // Update new transform pointer, and mark that transform is changed
    Matrix4* worldTransformPointer = entity->GetComponent<TransformComponent>()->GetWorldMatrixPtr();
    Light* light = entity->GetComponent<LightComponent>()->GetLightObject();
    light->SetPositionDirectionFromMatrix(*worldTransformPointer);
    entity->GetScene()->renderSystem->MarkForUpdate(light);
}

void LightUpdateSystem::AddEntity(Entity* entity)
{
    Light* lightObject = entity->GetComponent<LightComponent>()->GetLightObject();
    if (!lightObject)
        return;

    entityObjectMap[entity] = lightObject;
    GetScene()->GetRenderSystem()->AddLight(lightObject);

    RecalcLight(entity);
}

void LightUpdateSystem::RemoveEntity(Entity* entity)
{
    auto lightObjectIter = entityObjectMap.find(entity);

    if (lightObjectIter != entityObjectMap.end())
    {
        Light* lightObject = lightObjectIter->second;

        if (lightObject != nullptr)
        {
            GetScene()->GetRenderSystem()->RemoveLight(lightObject);
            entityObjectMap.erase(entity);
        }
    }
}

void LightUpdateSystem::PrepareForRemove()
{
    RenderSystem* renderSystem = GetScene()->GetRenderSystem();
    for (const auto& node : entityObjectMap)
    {
        renderSystem->RemoveLight(node.second);
    }
    entityObjectMap.clear();
}
};