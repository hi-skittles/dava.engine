#include "LandscapeSystem.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Scene.h"
#include "Render/Highlevel/Landscape.h"
#include "Math/Math2D.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"

namespace DAVA
{
LandscapeSystem::LandscapeSystem(Scene* scene)
    : SceneSystem(scene)
{
}

LandscapeSystem::~LandscapeSystem()
{
    DVASSERT(landscapeEntities.size() == 0);
}

void LandscapeSystem::AddEntity(Entity* entity)
{
    Landscape* landscapeObject = GetLandscape(entity);
    if (landscapeObject)
    {
        landscapeEntities.push_back(entity);

        const LandscapeQuality* quality = QualitySettingsSystem::Instance()->GetLandscapeQuality(QualitySettingsSystem::Instance()->GetCurLandscapeQuality());
        if (quality)
        {
            LandscapeSubdivision::SubdivisionMetrics& metrics = landscapeObject->GetSubdivision()->GetMetrics();
            metrics.normalMaxHeightError = quality->normalMaxHeightError;
            metrics.normalMaxPatchRadiusError = quality->normalMaxPatchRadiusError;
            metrics.normalMaxAbsoluteHeightError = quality->normalMaxAbsoluteHeightError;

            metrics.zoomMaxHeightError = quality->zoomMaxHeightError;
            metrics.zoomMaxPatchRadiusError = quality->zoomMaxPatchRadiusError;
            metrics.zoomMaxAbsoluteHeightError = quality->zoomMaxAbsoluteHeightError;

            if (quality->morphing && landscapeObject->renderMode != Landscape::RENDERMODE_INSTANCING_MORPHING)
            {
                landscapeObject->SetUseMorphing(quality->morphing);
            }
            else
            {
                landscapeObject->GetSubdivision()->UpdatePatchInfo(Rect2i(0, 0, -1, -1));
            }
        }
    }
}

void LandscapeSystem::RemoveEntity(Entity* entity)
{
    uint32 eCount = static_cast<uint32>(landscapeEntities.size());
    for (uint32 e = 0; e < eCount; ++e)
    {
        if (landscapeEntities[e] == entity)
        {
            RemoveExchangingWithLast(landscapeEntities, e);
            break;
        }
    }
}

void LandscapeSystem::PrepareForRemove()
{
    landscapeEntities.clear();
}

void LandscapeSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_LANDSCAPE_SYSTEM);

    for (Entity* e : landscapeEntities)
    {
        Landscape* landscapeObject = GetLandscape(e);
        if (landscapeObject->debugDrawMetrics)
            DrawPatchMetrics(landscapeObject, 0, 0, 0);
    }
}

Vector<Landscape*> LandscapeSystem::GetLandscapeObjects()
{
    Vector<Landscape*> landscapes(landscapeEntities.size());
    std::transform(landscapeEntities.begin(), landscapeEntities.end(), landscapes.begin(), [](Entity* e) { return GetLandscape(e); });

    return landscapes;
}

const Vector<Entity*>& LandscapeSystem::GetLandscapeEntities()
{
    return landscapeEntities;
}

void LandscapeSystem::DrawPatchMetrics(Landscape* landscape, uint32 level, uint32 x, uint32 y)
{
    const LandscapeSubdivision::SubdivisionPatchInfo& subdivPatchInfo = landscape->GetSubdivision()->GetPatchInfo(level, x, y);

    uint32 state = subdivPatchInfo.subdivisionState;
    if (state == LandscapeSubdivision::SubdivisionPatchInfo::CLIPPED)
        return;

    if (state == LandscapeSubdivision::SubdivisionPatchInfo::SUBDIVIDED)
    {
        uint32 x2 = x * 2;
        uint32 y2 = y * 2;

        DrawPatchMetrics(landscape, level + 1, x2 + 0, y2 + 0);
        DrawPatchMetrics(landscape, level + 1, x2 + 1, y2 + 0);
        DrawPatchMetrics(landscape, level + 1, x2 + 0, y2 + 1);
        DrawPatchMetrics(landscape, level + 1, x2 + 1, y2 + 1);
    }
    else
    {
        const LandscapeSubdivision::PatchQuadInfo& patch = landscape->GetSubdivision()->GetPatchQuadInfo(level, x, y);
        const LandscapeSubdivision::SubdivisionMetrics& metrics = landscape->GetSubdivision()->GetMetrics();

        Camera* camera = GetScene()->GetRenderSystem()->GetMainCamera();
        float32 tanFovY = tanf(camera->GetFOV() * PI / 360.f) / camera->GetAspect();

        float32 distance = Distance(camera->GetPosition(), patch.positionOfMaxError);
        float32 hError = Abs(patch.maxError) / (distance * tanFovY);

        Vector3 patchOrigin = patch.bbox.GetCenter();
        float32 patchDistance = Distance(camera->GetPosition(), patchOrigin);
        float32 rError = patch.radius / (patchDistance * tanFovY);

        float32 rErrorRel = Min(rError / landscape->GetSubdivision()->maxPatchRadiusError, 1.f);
        float32 hErrorRel = Min(hError / landscape->GetSubdivision()->maxHeightError, 1.f);

        RenderHelper* drawer = GetScene()->GetRenderSystem()->GetDebugDrawer();
        Color color;
        if (rErrorRel > hErrorRel)
        {
            color = Color(0.f, 0.f, 1.f, 1.f);
            drawer->DrawLine(patch.bbox.GetCenter(), patch.bbox.GetCenter() + Vector3(0.f, 0.f, patch.radius), color, RenderHelper::DRAW_WIRE_NO_DEPTH);
        }
        else
        {
            color = Color(1.f, 0.f, 0.f, 1.f);
            drawer->DrawLine(patch.positionOfMaxError - Vector3(0.f, 0.f, patch.maxError), patch.positionOfMaxError, color, RenderHelper::DRAW_WIRE_NO_DEPTH);
            float32 arrowToHeight = Max(patch.positionOfMaxError.z, patch.positionOfMaxError.z - patch.maxError) + patch.radius * .05f;
            Vector3 arrowTo = Vector3(patch.positionOfMaxError.x, patch.positionOfMaxError.y, arrowToHeight);
            drawer->DrawArrow(arrowTo + Vector3(0.f, 0.f, patch.radius * .2f), arrowTo, patch.radius * .05f, color, RenderHelper::DRAW_WIRE_NO_DEPTH);
        }

        float32 bboxMiddleH = (patch.bbox.min.z + patch.bbox.max.z) / 2.f;
        Vector3 p0(patch.bbox.min.x, patch.bbox.min.y, bboxMiddleH);
        Vector3 e1(patch.bbox.max.x - patch.bbox.min.x, 0.f, 0.f);
        Vector3 e2(0.f, patch.bbox.max.y - patch.bbox.min.y, 0.f);

        drawer->DrawLine(p0, p0 + e1, color, RenderHelper::DRAW_WIRE_NO_DEPTH);
        drawer->DrawLine(p0, p0 + e2, color, RenderHelper::DRAW_WIRE_NO_DEPTH);
        drawer->DrawLine(p0 + e1, p0 + e1 + e2, color, RenderHelper::DRAW_WIRE_NO_DEPTH);
        drawer->DrawLine(p0 + e2, p0 + e1 + e2, color, RenderHelper::DRAW_WIRE_NO_DEPTH);
    }
}
};
