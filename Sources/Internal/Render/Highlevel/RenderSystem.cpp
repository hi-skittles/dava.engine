#include "Render/Highlevel/RenderSystem.h"
#include "Scene3D/Entity.h"
#include "Render/Highlevel/RenderLayer.h"
#include "Render/Highlevel/RenderBatchArray.h"
#include "Render/Highlevel/RenderPass.h"
#include "Render/Highlevel/RenderBatch.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Render/Highlevel/Frustum.h"
#include "Render/Highlevel/Camera.h"
#include "Render/Highlevel/Light.h"
#include "Render/Highlevel/VisibilityQuadTree.h"
#include "Render/ShaderCache.h"

#include "Utils/Utils.h"

namespace DAVA
{
RenderSystem::RenderSystem()
{
    mainRenderPass = new MainForwardRenderPass(PASS_FORWARD);
    renderHierarchy = new QuadTree(10);
    markedObjects.reserve(100);
    debugDrawer = new RenderHelper();
    geoDecalManager = new GeoDecalManager();
}

RenderSystem::~RenderSystem()
{
    SafeRelease(mainCamera);
    SafeRelease(drawCamera);

    SafeRelease(globalMaterial);

    SafeDelete(renderHierarchy);
    SafeDelete(mainRenderPass);

    SafeDelete(debugDrawer);
    SafeDelete(geoDecalManager);
}

void RenderSystem::RenderPermanent(RenderObject* renderObject)
{
    DVASSERT(renderObject->GetRemoveIndex() == static_cast<uint32>(-1));

    /*on add calculate valid world bbox*/
    renderObject->Retain();
    renderObjectArray.push_back(renderObject);
    renderObject->SetRemoveIndex(static_cast<uint32>(renderObjectArray.size() - 1));

    AddRenderObject(renderObject);
}

void RenderSystem::RemoveFromRender(RenderObject* renderObject)
{
    DVASSERT(renderObject->GetRemoveIndex() != static_cast<uint32>(-1));

    FindAndRemoveExchangingWithLast(markedObjects, renderObject);
    renderObject->RemoveFlag(RenderObject::MARKED_FOR_UPDATE);

    RenderObject* lastRenderObject = renderObjectArray[renderObjectArray.size() - 1];
    renderObjectArray[renderObject->GetRemoveIndex()] = lastRenderObject;
    renderObjectArray.pop_back();
    lastRenderObject->SetRemoveIndex(renderObject->GetRemoveIndex());
    renderObject->SetRemoveIndex(-1);

    RemoveRenderObject(renderObject);

    renderObject->Release();
}

void RenderSystem::AddRenderObject(RenderObject* renderObject)
{
    renderObject->RecalculateWorldBoundingBox();
    renderHierarchy->AddRenderObject(renderObject);

    renderObject->SetRenderSystem(this);

    uint32 size = renderObject->GetRenderBatchCount();
    for (uint32 i = 0; i < size; ++i)
    {
        RenderBatch* batch = renderObject->GetRenderBatch(i);
        RegisterBatch(batch);
    }
}

void RenderSystem::RemoveRenderObject(RenderObject* renderObject)
{
    uint32 size = renderObject->GetRenderBatchCount();
    for (uint32 i = 0; i < size; ++i)
    {
        RenderBatch* batch = renderObject->GetRenderBatch(i);
        UnregisterBatch(batch);
    }

    geoDecalManager->RemoveRenderObject(renderObject);
    renderHierarchy->RemoveRenderObject(renderObject);

    renderObject->SetRenderSystem(nullptr);
}

void RenderSystem::PrebuildMaterial(NMaterial* material)
{
    //pre-build for all passes
    material->PreBuildMaterial(PASS_FORWARD);
}

void RenderSystem::RegisterBatch(RenderBatch* batch)
{
    RegisterMaterial(batch->GetMaterial());
}

void RenderSystem::UnregisterBatch(RenderBatch* batch)
{
    UnregisterMaterial(batch->GetMaterial());
}

void RenderSystem::RegisterMaterial(NMaterial* material)
{
    if (material == nullptr)
        return;

    // set globalMaterial to be parent for top material
    NMaterial* topParent = material->GetTopLevelParent();
    if (topParent != globalMaterial)
    {
        topParent->SetParent(globalMaterial);
    }

    PrebuildMaterial(material);
}

void RenderSystem::UnregisterMaterial(NMaterial* material)
{
    /*
    if (!material) return;

    while (material->GetParent() && material->GetParent() != globalMaterial)
    {
        material = material->GetParent();
    }

    if (material->GetParent())
    {
        material->SetParent(nullptr);
    }
    */
}

void RenderSystem::PrepareForShutdown()
{
    renderHierarchy->PrepareForShutdown();
}

void RenderSystem::SetGlobalMaterial(NMaterial* newGlobalMaterial)
{
    Set<DataNode*> dataNodes;
    for (RenderObject* obj : renderObjectArray)
    {
        obj->GetDataNodes(dataNodes);
    }
    for (DataNode* dataNode : dataNodes)
    {
        NMaterial* batchMaterial = dynamic_cast<NMaterial*>(dataNode);
        if (batchMaterial)
        {
            NMaterial* topMaterial = batchMaterial;
            while (topMaterial->GetParent() && topMaterial->GetParent() != globalMaterial && topMaterial->GetParent() != newGlobalMaterial)
            {
                topMaterial = topMaterial->GetParent();
            }
            topMaterial->SetParent(newGlobalMaterial);

            PrebuildMaterial(batchMaterial);
        }
    }

    SafeRelease(globalMaterial);
    globalMaterial = SafeRetain(newGlobalMaterial);
}

NMaterial* RenderSystem::GetGlobalMaterial() const
{
    return globalMaterial;
}

void RenderSystem::MarkForUpdate(RenderObject* renderObject)
{
    uint32 flags = renderObject->GetFlags();
    if (flags & RenderObject::MARKED_FOR_UPDATE)
        return;

    flags |= RenderObject::NEED_UPDATE;
    if ((flags & RenderObject::CLIPPING_VISIBILITY_CRITERIA) == RenderObject::CLIPPING_VISIBILITY_CRITERIA)
    {
        markedObjects.push_back(renderObject);
        flags |= RenderObject::MARKED_FOR_UPDATE;
    }
    renderObject->SetFlags(flags);
}

void RenderSystem::MarkForUpdate(Light* lightNode)
{
    movedLights.push_back(lightNode);
}

void RenderSystem::RegisterForUpdate(IRenderUpdatable* updatable)
{
    objectsForUpdate.push_back(updatable);
}

void RenderSystem::UnregisterFromUpdate(IRenderUpdatable* updatable)
{
    uint32 size = static_cast<uint32>(objectsForUpdate.size());
    for (uint32 i = 0; i < size; ++i)
    {
        if (objectsForUpdate[i] == updatable)
        {
            objectsForUpdate[i] = objectsForUpdate[size - 1];
            objectsForUpdate.pop_back();
            return;
        }
    }
}

void RenderSystem::UpdateNearestLights(RenderObject* renderObject)
{
    Light* nearestLight = nullptr;
    float32 squareMinDistance = std::numeric_limits<float>::max();
    Vector3 position = renderObject->GetWorldBoundingBox().GetCenter();

    if (lights.size() == 1)
    {
        nearestLight = (lights.front() && lights.front()->IsDynamic()) ? lights.front() : nullptr;
    }
    else
    {
        for (Light* light : lights)
        {
            if (light->IsDynamic())
            {
                const Vector3& lightPosition = light->GetPosition();
                float32 squareDistanceToLight = (position - lightPosition).SquareLength();
                if ((!nearestLight) || (squareDistanceToLight < squareMinDistance))
                {
                    squareMinDistance = squareDistanceToLight;
                    nearestLight = light;
                }
            }
        }
    }

    renderObject->SetLight(0, nearestLight);
}

void RenderSystem::FindNearestLights()
{
    size_t size = renderObjectArray.size();
    for (size_t k = 0; k < size; ++k)
    {
        UpdateNearestLights(renderObjectArray[k]);
    }
}

void RenderSystem::AddLight(Light* light)
{
    lights.push_back(SafeRetain(light));
    FindNearestLights();
}

void RenderSystem::RemoveLight(Light* light)
{
    FindAndRemoveExchangingWithLast(lights, light);
    FindNearestLights();

    SafeRelease(light);
}

Vector<Light*>& RenderSystem::GetLights()
{
    return lights;
}

void RenderSystem::SetForceUpdateLights()
{
    forceUpdateLights = true;
}

void RenderSystem::Update(float32 timeElapsed)
{
    if (!hierarchyInitialized)
    {
        renderHierarchy->Initialize();
        hierarchyInitialized = true;
    }

    for (RenderObject* obj : markedObjects)
    {
        obj->RecalculateWorldBoundingBox();
        UpdateNearestLights(obj);

        if (obj->GetTreeNodeIndex() != QuadTree::INVALID_TREE_NODE_INDEX)
            renderHierarchy->ObjectUpdated(obj);

        obj->RemoveFlag(RenderObject::NEED_UPDATE | RenderObject::MARKED_FOR_UPDATE);
    }
    markedObjects.clear();

    renderHierarchy->Update();

    if (movedLights.size() > 0 || forceUpdateLights)
    {
        FindNearestLights();
        forceUpdateLights = false;
        movedLights.clear();
    }

    uint32 size = static_cast<uint32>(objectsForUpdate.size());
    for (uint32 i = 0; i < size; ++i)
    {
        objectsForUpdate[i]->RenderUpdate(mainCamera, timeElapsed);
    }
}

void RenderSystem::DebugDrawHierarchy(const Matrix4& cameraMatrix)
{
    if (renderHierarchy)
        renderHierarchy->DebugDraw(cameraMatrix, debugDrawer);
}

void RenderSystem::Render()
{
    rhi::RenderPassConfig& config = mainRenderPass->GetPassConfig();

    const FastName& currentMSAA = QualitySettingsSystem::Instance()->GetCurMSAAQuality();
    rhi::AntialiasingType aaType = currentMSAA.IsValid() ? QualitySettingsSystem::Instance()->GetMSAAQuality(currentMSAA)->type : rhi::AntialiasingType::NONE;
    config.antialiasingType = (allowAntialiasing && rhi::DeviceCaps().SupportsAntialiasingType(aaType)) ? aaType : rhi::AntialiasingType::NONE;

    config.colorBuffer[0].storeAction = (config.UsingMSAA()) ? rhi::STOREACTION_RESOLVE : rhi::STOREACTION_STORE;
    config.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
    config.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;

    mainRenderPass->Draw(this);
}

void RenderSystem::SetAntialiasingAllowed(bool allow)
{
    allowAntialiasing = allow;
}

void RenderSystem::SetMainRenderTarget(rhi::HTexture color, rhi::HTexture depthStencil, rhi::LoadAction colorLoadAction, const Color& clearColor)
{
    rhi::RenderPassConfig& config = mainRenderPass->GetPassConfig();
    config.colorBuffer[0].texture = color;
    config.colorBuffer[0].loadAction = colorLoadAction;
    config.colorBuffer[0].clearColor[0] = clearColor.r;
    config.colorBuffer[0].clearColor[1] = clearColor.g;
    config.colorBuffer[0].clearColor[2] = clearColor.b;
    config.colorBuffer[0].clearColor[3] = clearColor.a;
    config.depthStencilBuffer.texture = depthStencil;
}

void RenderSystem::SetMainPassProperties(uint32 priority, const Rect& viewport, uint32 width, uint32 height, PixelFormat format)
{
    mainRenderPass->SetViewport(viewport);
    mainRenderPass->SetRenderTargetProperties(width, height, format);
    mainRenderPass->GetPassConfig().priority = priority;
}

rhi::RenderPassConfig& RenderSystem::GetMainPassConfig()
{
    return mainRenderPass->GetPassConfig();
}
};