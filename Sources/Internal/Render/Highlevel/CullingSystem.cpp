#include "Render/Highlevel/CullingSystem.h"
#include "Scene3D/Entity.h"
#include "Render/Highlevel/RenderLayer.h"
#include "Render/Highlevel/RenderPass.h"
#include "Render/Highlevel/RenderBatch.h"
#include "Render/Highlevel/Camera.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Render/Highlevel/Frustum.h"
#include "Logger/Logger.h"

namespace DAVA
{
CullingSystem::CullingSystem(Scene* scene)
    : SceneSystem(scene)
{
}

CullingSystem::~CullingSystem()
{
}

void CullingSystem::ImmediateUpdate(Entity* entity)
{
    RenderObject* renderObject = GetRenderObject(entity);
    if (!renderObject)
        return;

    if (renderObject->GetRemoveIndex() == static_cast<uint32>(-1)) // FAIL, SHOULD NOT HAPPEN
    {
        Logger::Error("Object in entity was replaced suddenly. ");
    }

    // Do we need updates???
}

void CullingSystem::AddEntity(Entity* entity)
{
}

void CullingSystem::RemoveEntity(Entity* entity)
{
}

void CullingSystem::SetCamera(Camera* _camera)
{
    camera = _camera;
}

void CullingSystem::Process(float32 timeElapsed)
{
    int32 objectsCulled = 0;

    //Frustum * frustum = camera->GetFrustum();

    uint32 size = static_cast<uint32>(renderObjectArray.size());
    for (uint32 pos = 0; pos < size; ++pos)
    {
        //RenderObject * node = renderObjectArray[pos];
        //node->AddFlag(RenderObject::VISIBLE_AFTER_CLIPPING_THIS_FRAME);
        //Logger::FrameworkDebug("Cull Node: %s rc: %d", node->GetFullName().c_str(), node->GetRetainCount());
        //if (!frustum->IsInside(node->GetWorldTransformedBox()))
        {
            //node->RemoveFlag(RenderObject::VISIBLE_AFTER_CLIPPING_THIS_FRAME);
            objectsCulled++;
        }
    }
}
};
