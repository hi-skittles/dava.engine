#include "VisibilityCheckSystem.h"
#include "VisibilityCheckRenderer.h"

#include "Constants.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Components/VisibilityCheckComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"

#include "Math/MathHelpers.h"
#include "Utils/Random.h"

#include <Render/2D/Systems/RenderSystem2D.h>
#include <Render/Highlevel/Landscape.h>
#include <Render/Highlevel/RenderSystem.h>
#include <Render/PixelFormatDescriptor.h>
#include <Render/Renderer.h>

namespace VCSInternal
{
static const DAVA::uint32 CUBEMAPS_POOL_SIZE = 1;
static const DAVA::uint32 CUBEMAP_SIZE = 2048;
static DAVA::Array<DAVA::Texture*, CUBEMAPS_POOL_SIZE> cubemapPool;

DAVA::Texture* CubemapRenderTargetAtIndex(DAVA::uint32 index)
{
    DVASSERT(index < CUBEMAPS_POOL_SIZE);
    if (cubemapPool[index] == nullptr)
    {
        const DAVA::PixelFormatDescriptor& pfd = DAVA::PixelFormatDescriptor::GetPixelFormatDescriptor(VisibilityCheckRenderer::TEXTURE_FORMAT);
        DVASSERT(rhi::TextureFormatSupported(pfd.format, rhi::PROG_FRAGMENT));
        cubemapPool[index] = DAVA::Texture::CreateFBO(CUBEMAP_SIZE, CUBEMAP_SIZE, VisibilityCheckRenderer::TEXTURE_FORMAT, true, rhi::TEXTURE_TYPE_CUBE);
        cubemapPool[index]->SetMinMagFilter(rhi::TextureFilter::TEXFILTER_LINEAR, rhi::TextureFilter::TEXFILTER_LINEAR, rhi::TextureMipFilter::TEXMIPFILTER_NONE);
    }
    return cubemapPool[index];
}
}

VisibilityCheckSystem::VisibilityCheckSystem(DAVA::Scene* scene)
    : DAVA::SceneSystem(scene)
    , debugMaterial(new DAVA::NMaterial())
{
    renderer.SetDelegate(this);
    debugMaterial->SetFXName(DAVA::FastName("~res:/ResourceEditor/LandscapeEditor/Materials/Distance.Debug2D.material"));
    debugMaterial->PreBuildMaterial(DAVA::PASS_FORWARD);
}

VisibilityCheckSystem::~VisibilityCheckSystem() = default;

DAVA::Camera* VisibilityCheckSystem::GetFinalGatherCamera() const
{
    return GetScene()->GetDrawCamera();
}

DAVA::Camera* VisibilityCheckSystem::GetRenderCamera() const
{
    return GetScene()->GetCurrentCamera();
}

void VisibilityCheckSystem::ReleaseCubemapRenderTargets()
{
    for (auto& cb : VCSInternal::cubemapPool)
    {
        DAVA::SafeRelease(cb);
    }
}

void VisibilityCheckSystem::Recalculate()
{
    shouldPrerender = true;
    if (renderer.FrameFixed())
    {
        shouldFixFrame = true;
    }
}

void VisibilityCheckSystem::RegisterEntity(DAVA::Entity* entity)
{
    auto visibilityComponent = entity->GetComponent<DAVA::VisibilityCheckComponent>();
    auto renderComponent = DAVA::GetRenderComponent(entity);
    if ((renderComponent != nullptr) || (visibilityComponent != nullptr))
    {
        AddEntity(entity);
    }
}

void VisibilityCheckSystem::UnregisterEntity(DAVA::Entity* entity)
{
    auto visibilityComponent = entity->GetComponent<DAVA::VisibilityCheckComponent>();
    auto renderComponent = DAVA::GetRenderComponent(entity);
    if ((renderComponent != nullptr) || (visibilityComponent != nullptr))
    {
        RemoveEntity(entity);
    }
}

void VisibilityCheckSystem::AddEntity(DAVA::Entity* entity)
{
    auto requiredComponent = entity->GetComponent<DAVA::VisibilityCheckComponent>();
    if (requiredComponent != nullptr)
    {
        requiredComponent->Invalidate();
        entitiesWithVisibilityComponent.insert({ entity, DAVA::Vector<DAVA::Vector3>() });
        shouldPrerender = true;
        forceRebuildPoints = true;
    }

    auto renderComponent = DAVA::GetRenderComponent(entity);
    if (renderComponent != nullptr)
    {
        auto ro = renderComponent->GetRenderObject();
        if (ro->GetType() == DAVA::RenderObject::TYPE_LANDSCAPE)
        {
            landscape = static_cast<DAVA::Landscape*>(ro);
        }
        renderObjectToEntity[ro] = entity;
    }
}

void VisibilityCheckSystem::RemoveEntity(DAVA::Entity* entity)
{
    auto i = std::find_if(entitiesWithVisibilityComponent.begin(), entitiesWithVisibilityComponent.end(), [entity](const EntityMap::value_type& item) {
        return item.first == entity;
    });

    if (i != entitiesWithVisibilityComponent.end())
    {
        entitiesWithVisibilityComponent.erase(i);
        shouldPrerender = true;
        forceRebuildPoints = true;
    }

    auto renderComponent = DAVA::GetRenderComponent(entity);
    if (renderComponent != nullptr)
    {
        auto ro = renderComponent->GetRenderObject();
        if (ro->GetType() == DAVA::RenderObject::TYPE_LANDSCAPE)
        {
            landscape = nullptr;
        }
        renderObjectToEntity.erase(ro);
    }
}

void VisibilityCheckSystem::PrepareForRemove()
{
    entitiesWithVisibilityComponent.clear();
    renderObjectToEntity.clear();
    landscape = nullptr;
    shouldPrerender = true;
    forceRebuildPoints = true;
}

void VisibilityCheckSystem::Process(DAVA::float32 timeElapsed)
{
    if (!DAVA::Renderer::GetOptions()->IsOptionEnabled(DAVA::RenderOptions::DEBUG_ENABLE_VISIBILITY_SYSTEM))
    {
        return;
    }

    bool shouldRebuildIndices = false;

    for (auto& mapItem : entitiesWithVisibilityComponent)
    {
        auto visibilityComponent = mapItem.first->GetComponent<DAVA::VisibilityCheckComponent>();
        if (!visibilityComponent->IsValid() || forceRebuildPoints)
        {
            if (visibilityComponent->ShouldRebuildPoints() || forceRebuildPoints)
            {
                BuildPointSetForEntity(mapItem);
                shouldRebuildIndices = true;
            }
            shouldPrerender = true;
            visibilityComponent->SetValid();
        }
    }

    if (shouldRebuildIndices || forceRebuildPoints)
    {
        BuildIndexSet();
    }
    UpdatePointSet();
}

void VisibilityCheckSystem::Draw()
{
    if (!DAVA::Renderer::GetOptions()->IsOptionEnabled(DAVA::RenderOptions::DEBUG_ENABLE_VISIBILITY_SYSTEM))
    {
        return;
    }

    bool enableDebug = false;

    bool shouldRenderOverlay = false;
    DAVA::RenderSystem* rs = GetScene()->GetRenderSystem();
    DAVA::RenderHelper* dbg = rs->GetDebugDrawer();
    for (const auto& mapItem : entitiesWithVisibilityComponent)
    {
        DAVA::VisibilityCheckComponent* visibilityComponent = mapItem.first->GetComponent<DAVA::VisibilityCheckComponent>();

        enableDebug |= visibilityComponent->GetDebugDrawEnabled();

        if (visibilityComponent->IsEnabled())
        {
            const DAVA::Matrix4& worldTransform = mapItem.first->GetWorldTransform();
            DAVA::Vector3 position = worldTransform.GetTranslationVector();
            DAVA::Vector3 direction = MultiplyVectorMat3x3(DAVA::Vector3(0.0f, 0.0f, 1.0f), worldTransform);
            dbg->DrawCircle(position, direction, visibilityComponent->GetRadius(), 36, DAVA::Color::White, DAVA::RenderHelper::DRAW_WIRE_DEPTH);
            shouldRenderOverlay = true;
        }
    }

    if (!CacheIsValid())
    {
        BuildCache();
        shouldPrerender = true;
    }

    if (shouldPrerender)
    {
        Prerender();
    }

    for (const auto& point : controlPoints)
    {
        dbg->DrawIcosahedron(point.point, VisibilityCheckRenderer::cameraNearClipPlane,
                             DAVA::Color(1.0f, 1.0f, 0.5f, 1.0f), DAVA::RenderHelper::DRAW_WIRE_DEPTH);
        dbg->DrawLine(point.point, point.point + 2.0f * point.normal, DAVA::Color(0.25f, 1.0f, 0.25f, 1.0f));
    }

    DAVA::uint32 previousPointIndex = static_cast<DAVA::uint32>(currentPointIndex);
    for (DAVA::uint32 cm = 0; (cm < VCSInternal::CUBEMAPS_POOL_SIZE) && (currentPointIndex < controlPoints.size()); ++cm, ++currentPointIndex)
    {
        DAVA::uint32 pointIndex = controlPointIndices[currentPointIndex];
        const auto& point = controlPoints[pointIndex];
        auto cubemap = VCSInternal::CubemapRenderTargetAtIndex(cm);
        renderer.RenderToCubemapFromPoint(rs, point.point, cubemap);
        renderer.RenderVisibilityToTexture(rs, GetRenderCamera(), GetFinalGatherCamera(), cubemap, point);
    }

    if (shouldFixFrame && (currentPointIndex == controlPoints.size()) && (previousPointIndex < controlPoints.size()))
    {
        renderer.FixFrame();
        shouldFixFrame = false;
    }

    if (shouldRenderOverlay)
    {
        if ((currentPointIndex == controlPoints.size()) || renderer.FrameFixed())
        {
            renderer.RenderCurrentOverlayTexture(rs, GetFinalGatherCamera());
        }

        if (currentPointIndex < controlPoints.size())
        {
            float progress = static_cast<float>(currentPointIndex) / static_cast<float>(controlPoints.size());
            renderer.RenderProgress(progress, shouldFixFrame ? DAVA::Color(0.25f, 0.5f, 1.0f, 1.0f) : DAVA::Color::White);
        }
    }

    if (enableDebug)
    {
        DAVA::Texture* cubemap = VCSInternal::CubemapRenderTargetAtIndex(0);
        if (cubemap)
            DAVA::RenderSystem2D::Instance()->DrawTexture(cubemap, debugMaterial, DAVA::Color::White, DAVA::Rect(2.0f, 2.0f, DAVA::float32(stateCache.viewportSize.dx) - 4.f, 512.f));

        if (renderer.renderTarget)
            DAVA::RenderSystem2D::Instance()->DrawTexture(renderer.renderTarget, DAVA::RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL, DAVA::Color::White, DAVA::Rect(2.f, 516.f, 256.f, 256.f));

        if (renderer.fixedFrame)
            DAVA::RenderSystem2D::Instance()->DrawTexture(renderer.fixedFrame, DAVA::RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL, DAVA::Color::White, DAVA::Rect(260.f, 516.f, 256.f, 256.f));

        if (renderer.reprojectionTexture)
            DAVA::RenderSystem2D::Instance()->DrawTexture(renderer.reprojectionTexture, DAVA::RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL, DAVA::Color::White, DAVA::Rect(520.f, 516.f, 256.f, 256.f));
    }

    GetFinalGatherCamera()->SetupDynamicParameters(false, nullptr);
}

void VisibilityCheckSystem::InvalidateMaterials()
{
    renderer.InvalidateMaterials();
}

void VisibilityCheckSystem::UpdatePointSet()
{
    controlPoints.clear();

    for (const auto& mapItem : entitiesWithVisibilityComponent)
    {
        auto entity = mapItem.first;
        auto visibilityComponent = entity->GetComponent<DAVA::VisibilityCheckComponent>();
        if (visibilityComponent->IsEnabled())
        {
            auto worldTransform = entity->GetWorldTransform();
            DAVA::Vector3 position = worldTransform.GetTranslationVector();
            DAVA::Vector3 normal = MultiplyVectorMat3x3(DAVA::Vector3(0.0f, 0.0f, 1.0f), worldTransform);
            normal.Normalize();
            DAVA::float32 upAngle = std::cos(DAVA::DegToRad(90.0f - visibilityComponent->GetUpAngle()));
            DAVA::float32 dnAngle = -std::cos(DAVA::DegToRad(90.0f - visibilityComponent->GetDownAngle()));
            DAVA::float32 maxDist = visibilityComponent->GetMaximumDistance();
            DAVA::float32 snapHeight = visibilityComponent->GetHeightAboveLandscape();

            for (const auto& pt : mapItem.second)
            {
                DAVA::Vector3 placedPoint;
                DAVA::Vector3 placedNormal;
                DAVA::Vector3 transformedPoint = position + MultiplyVectorMat3x3(pt, worldTransform);
                if ((landscape != nullptr) && landscape->PlacePoint(transformedPoint, placedPoint, &placedNormal))
                {
                    if (visibilityComponent->ShouldPlaceOnLandscape())
                    {
                        normal = placedNormal;
                        transformedPoint.z = placedPoint.z + snapHeight;
                    }
                    transformedPoint.z = DAVA::Max(transformedPoint.z, placedPoint.z + 2.0f * VisibilityCheckRenderer::cameraNearClipPlane);
                }

                controlPoints.emplace_back(transformedPoint, normal, GetNormalizedColorForEntity(mapItem), upAngle, dnAngle, maxDist);
            }
        }
    }
}

void VisibilityCheckSystem::Prerender()
{
    renderer.CreateOrUpdateRenderTarget(stateCache.viewportSize);
    renderer.PreRenderScene(GetScene()->GetRenderSystem(), GetFinalGatherCamera());

    currentPointIndex = 0;
    shouldPrerender = false;
}

bool VisibilityCheckSystem::CacheIsValid()
{
    DAVA::Size2i vpSize(DAVA::Renderer::GetFramebufferWidth(), DAVA::Renderer::GetFramebufferHeight());
    if (vpSize != stateCache.viewportSize)
        return false;

    auto cam = GetFinalGatherCamera();
    if (cam != stateCache.camera)
        return false;

    auto currentMatrix = cam->GetViewProjMatrix();
    if (Memcmp(currentMatrix.data, stateCache.viewprojMatrix.data, sizeof(DAVA::Matrix4)) != 0)
        return false;

    return true;
}

void VisibilityCheckSystem::BuildCache()
{
    stateCache.camera = GetFinalGatherCamera();
    DVASSERT(stateCache.camera != nullptr);

    stateCache.viewportSize = DAVA::Size2i(DAVA::Renderer::GetFramebufferWidth(), DAVA::Renderer::GetFramebufferHeight());
    stateCache.viewprojMatrix = stateCache.camera->GetViewProjMatrix();
}

bool VisibilityCheckSystem::ShouldDrawRenderObject(DAVA::RenderObject* object)
{
    auto type = object->GetType();

    if (type == DAVA::RenderObject::TYPE_LANDSCAPE)
        return true;

    if ((type == DAVA::RenderObject::TYPE_SPEED_TREE) || (type == DAVA::RenderObject::TYPE_SPRITE) ||
        (type == DAVA::RenderObject::TYPE_VEGETATION) || (type == DAVA::RenderObject::TYPE_PARTICLE_EMITTER))
    {
        return false;
    }

    auto entityIterator = renderObjectToEntity.find(object);
    if (entityIterator == renderObjectToEntity.end())
        return false;

    DAVA::String collisionTypeString = "CollisionType";
    if ((object->GetMaxSwitchIndex() > 0) && (object->GetSwitchIndex() > 0))
    {
        collisionTypeString = "CollisionTypeCrashed";
    }

    DAVA::VariantType* collisionValue = GetCustomPropertiesValueRecursive(entityIterator->second, collisionTypeString);
    if ((collisionValue == nullptr) || (collisionValue->type != DAVA::VariantType::TYPE_INT32))
        return false;

    const DAVA::int32 collisiontype = collisionValue->AsInt32();
    if ((ResourceEditor::ESOT_NO_COLISION == collisiontype) ||
        (ResourceEditor::ESOT_TREE == collisiontype) ||
        (ResourceEditor::ESOT_BUSH == collisiontype) ||
        (ResourceEditor::ESOT_FALLING == collisiontype) ||
        (ResourceEditor::ESOT_FRAGILE_PROJ_INV == collisiontype) ||
        (ResourceEditor::ESOT_SPEED_TREE == collisiontype))
    {
        return false;
    }

    return true;
}

namespace VCSLocal
{
inline bool DistanceFromPointToAnyPointFromSetGreaterThan(const DAVA::Vector3& pt,
                                                          const DAVA::Vector<DAVA::Vector3>& points, DAVA::float32 distanceSquared, DAVA::float32 radiusSquared)
{
    DAVA::float32 distanceFromCenter = pt.x * pt.x + pt.y * pt.y;
    if (distanceFromCenter > radiusSquared)
        return false;

    for (const auto& e : points)
    {
        DAVA::float32 dx = e.x - pt.x;
        DAVA::float32 dy = e.y - pt.y;
        if (dx * dx + dy * dy < distanceSquared)
            return false;
    }

    return true;
}

bool TryToGenerateAroundPoint(const DAVA::Vector3& src, DAVA::float32 distanceBetweenPoints, DAVA::float32 radius, DAVA::Vector<DAVA::Vector3>& points)
{
    const DAVA::uint32 maxAttempts = 36;

    DAVA::float32 distanceSquared = distanceBetweenPoints * distanceBetweenPoints;
    DAVA::float32 radiusSquared = radius * radius;
    DAVA::float32 angle = DAVA::Random::Instance()->RandFloat32InBounds(-DAVA::PI, DAVA::PI);
    DAVA::float32 da = 2.0f * DAVA::PI / static_cast<float>(maxAttempts);
    DAVA::uint32 attempts = 0;
    DAVA::Vector3 newPoint;
    bool canInclude = false;
    do
    {
        newPoint = src + DAVA::Polar(angle, 2.0f * distanceBetweenPoints);
        if (DistanceFromPointToAnyPointFromSetGreaterThan(newPoint, points, distanceSquared, radiusSquared))
        {
            points.push_back(newPoint);
            return true;
        }
        angle += da;
    } while ((++attempts < maxAttempts) && !canInclude);

    return false;
};
}

void VisibilityCheckSystem::BuildPointSetForEntity(EntityMap::value_type& item)
{
    auto component = item.first->GetComponent<DAVA::VisibilityCheckComponent>();

    DAVA::float32 radius = component->GetRadius();
    DAVA::float32 radiusSquared = radius * radius;
    DAVA::float32 distanceBetweenPoints = component->GetDistanceBetweenPoints();
    DAVA::float32 distanceSquared = distanceBetweenPoints * distanceBetweenPoints;

    DAVA::uint32 pointsToGenerate = 2 * static_cast<DAVA::uint32>(radiusSquared / distanceSquared);
    item.second.clear();
    item.second.reserve(pointsToGenerate);

    if (pointsToGenerate < 2)
    {
        item.second.emplace_back(0.0f, 0.0f, 0.0f);
    }
    else
    {
        item.second.push_back(DAVA::Polar(DAVA::Random::Instance()->RandFloat32InBounds(-DAVA::PI, +DAVA::PI), radius - distanceBetweenPoints));

        bool canGenerate = true;
        while (canGenerate)
        {
            canGenerate = false;
            for (DAVA::int32 i = static_cast<DAVA::int32>(item.second.size()) - 1; i >= 0; --i)
            {
                if (VCSLocal::TryToGenerateAroundPoint(item.second.at(i), distanceBetweenPoints, radius, item.second))
                {
                    canGenerate = true;
                    break;
                }
            }
        }
    }

    DAVA::float32 verticalVariance = component->GetVerticalVariance();
    if (verticalVariance > 0.0f)
    {
        for (auto& p : item.second)
        {
            p.z = DAVA::Random::Instance()->RandFloat32InBounds(-verticalVariance, verticalVariance);
        }
    }
}

void VisibilityCheckSystem::BuildIndexSet()
{
    size_t totalPoints = 0;
    for (const auto& item : entitiesWithVisibilityComponent)
    {
        auto visibilityComponent = item.first->GetComponent<DAVA::VisibilityCheckComponent>();
        if (visibilityComponent->IsEnabled())
        {
            totalPoints += item.second.size();
        }
    }
    controlPointIndices.resize(totalPoints);
    for (size_t i = 0; i < totalPoints; ++i)
    {
        controlPointIndices[i] = static_cast<DAVA::uint32>(i);
    }

    if (totalPoints > 0)
    {
        std::random_shuffle(controlPointIndices.begin() + 1, controlPointIndices.end());
    }

    forceRebuildPoints = false;
}

DAVA::Color VisibilityCheckSystem::GetNormalizedColorForEntity(const EntityMap::value_type& item) const
{
    auto component = item.first->GetComponent<DAVA::VisibilityCheckComponent>();
    DAVA::Color normalizedColor = component->GetColor();
    if (component->ShouldNormalizeColor())
    {
        DAVA::float32 fpoints = static_cast<float>(item.second.size());
        normalizedColor.r = (normalizedColor.r > 0.0f) ? std::max(1.0f / 255.0f, normalizedColor.r / fpoints) : 0.0f;
        normalizedColor.g = (normalizedColor.g > 0.0f) ? std::max(1.0f / 255.0f, normalizedColor.g / fpoints) : 0.0f;
        normalizedColor.b = (normalizedColor.b > 0.0f) ? std::max(1.0f / 255.0f, normalizedColor.b / fpoints) : 0.0f;
    }
    return normalizedColor;
}

void VisibilityCheckSystem::FixCurrentFrame()
{
    if (currentPointIndex == controlPoints.size())
    {
        renderer.FixFrame();
    }
    else
    {
        shouldFixFrame = true;
    }
}

void VisibilityCheckSystem::ReleaseFixedFrame()
{
    renderer.ReleaseFrame();
    shouldFixFrame = false;
}
