#include "Render/Highlevel/Heightmap.h"
#include "Render/Highlevel/LandscapeSubdivision.h"
#include "Render/Highlevel/Frustum.h"
#include "Render/Highlevel/Camera.h"
#include "Render/RHI/rhi_Public.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(LandscapeSubdivision::SubdivisionMetrics)
{
    ReflectionRegistrator<SubdivisionMetrics>::Begin()
    .Field("normalMaxHeightError", &SubdivisionMetrics::normalMaxHeightError)[M::DisplayName("Maximum height error"), M::Group("Normal")]
    .Field("normalMaxPatchRadiusError", &SubdivisionMetrics::normalMaxPatchRadiusError)[M::DisplayName("Maximum patch radius error"), M::Group("Normal")]
    .Field("normalMaxAbsoluteHeightError", &SubdivisionMetrics::normalMaxAbsoluteHeightError)[M::DisplayName("Maximum absolute height error"), M::Group("Normal")]
    .Field("zoomMaxHeightError", &SubdivisionMetrics::zoomMaxHeightError)[M::DisplayName("Maximum height error"), M::Group("Zoom")]
    .Field("zoomMaxPatchRadiusError", &SubdivisionMetrics::zoomMaxPatchRadiusError)[M::DisplayName("Maximum patch radius error"), M::Group("Zoom")]
    .Field("zoomMaxAbsoluteHeightError", &SubdivisionMetrics::zoomMaxAbsoluteHeightError)[M::DisplayName("Maximum absolute height error"), M::Group("Zoom")]
    .End();
}

DAVA_VIRTUAL_REFLECTION_IMPL(LandscapeSubdivision)
{
    ReflectionRegistrator<LandscapeSubdivision>::Begin()
    .Field("metrics", &LandscapeSubdivision::metrics)[M::DisplayName("Metrics")]
    .End();
}

template <>
bool AnyCompare<LandscapeSubdivision::SubdivisionMetrics>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2)
{
    return v1.Get<LandscapeSubdivision::SubdivisionMetrics>() == v2.Get<LandscapeSubdivision::SubdivisionMetrics>();
}

bool LandscapeSubdivision::SubdivisionMetrics::operator==(const SubdivisionMetrics& other) const
{
    return normalFov == other.normalFov &&
    zoomFov == other.zoomFov &&
    normalMaxHeightError == other.normalMaxHeightError &&
    normalMaxPatchRadiusError == other.normalMaxPatchRadiusError &&
    normalMaxAbsoluteHeightError == other.normalMaxAbsoluteHeightError &&
    zoomMaxHeightError == other.zoomMaxHeightError &&
    zoomMaxPatchRadiusError == other.zoomMaxPatchRadiusError &&
    zoomMaxAbsoluteHeightError == other.zoomMaxAbsoluteHeightError;
}

LandscapeSubdivision::LandscapeSubdivision()
{
    frustum = new Frustum();
}

LandscapeSubdivision::~LandscapeSubdivision()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    SafeRelease(frustum);
    SafeRelease(heightmap);
}

void LandscapeSubdivision::ReleaseInternalData()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    subdivLevelInfoArray.clear();
    patchQuadArray.clear();
    subdivPatchArray.clear();

    SafeRelease(heightmap);
}

void LandscapeSubdivision::PrepareSubdivision(Camera* camera, const Matrix4* worldTransform)
{
    ++updateID;

    cameraPos = camera->GetPosition();

    frustum->Build((*worldTransform) * camera->GetViewProjMatrix(), rhi::DeviceCaps().isZeroBaseClipRange);

    float32 fovLerp = Clamp((camera->GetFOV() - metrics.zoomFov) / (metrics.normalFov - metrics.zoomFov), 0.f, 1.f);
    maxHeightError = metrics.zoomMaxHeightError + (metrics.normalMaxHeightError - metrics.zoomMaxHeightError) * fovLerp;
    maxPatchRadiusError = metrics.zoomMaxPatchRadiusError + (metrics.normalMaxPatchRadiusError - metrics.zoomMaxPatchRadiusError) * fovLerp;
    maxAbsoluteHeightError = metrics.zoomMaxAbsoluteHeightError + (metrics.normalMaxAbsoluteHeightError - metrics.zoomMaxAbsoluteHeightError) * fovLerp;

    tanFovY = tanf(camera->GetFOV() * PI / 360.f) / camera->GetAspect();
    //used for calculate metrics projection on screen. Projection calculate as '1.0 / (distance * tan(fov / 2))'. See errors calculation in SubdividePatch()

    terminatedPatchesCount = 0;
    SubdividePatch(0, 0, 0, 0x3f, maxHeightError, maxPatchRadiusError);
}

void LandscapeSubdivision::UpdatePatchInfo(const Rect2i& heighmapRect)
{
    UpdatePatchInfo(0, 0, 0, nullptr, heighmapRect);
}

void LandscapeSubdivision::UpdatePatchInfo(uint32 level, uint32 x, uint32 y, PatchQuadInfo* parentPatch, const Rect2i& updateRect)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    if (level >= subdivLevelCount)
        return;

    int32 hmSize = heightmap->Size();
    uint32 patchSize = hmSize >> level;

    uint32 xx = x * patchSize;
    uint32 yy = y * patchSize;

    if (updateRect.dx >= 0 && updateRect.dy >= 0 && !Rect2i(xx, yy, patchSize, patchSize).RectIntersects(updateRect))
        return;

    SubdivisionLevelInfo& levelInfo = subdivLevelInfoArray[level];
    PatchQuadInfo* patch = &patchQuadArray[levelInfo.offset + (y << level) + x];

    patch->maxError = 0.f;
    patch->positionOfMaxError = Vector3();
    patch->bbox = AABBox3();

    uint32 step = patchSize / patchSizeQuads;
    DVASSERT(step);

    for (uint32 y0 = yy; y0 < yy + patchSize; y0 += step)
    {
        for (uint32 x0 = xx; x0 < xx + patchSize; x0 += step)
        {
            uint32 x_ = x0 + (step >> 1);
            uint32 y_ = y0 + (step >> 1);
            uint32 x1 = x0 + step;
            uint32 y1 = y0 + step;

            //Patch corners points
            Vector3 p00 = heightmap->GetPoint(x0, y0, bbox);
            Vector3 p01 = heightmap->GetPoint(x0, y1, bbox);
            Vector3 p10 = heightmap->GetPoint(x1, y0, bbox);
            Vector3 p11 = heightmap->GetPoint(x1, y1, bbox);

            //Add to bbox only corners points
            patch->bbox.AddPoint(p00);
            patch->bbox.AddPoint(p01);
            patch->bbox.AddPoint(p10);
            patch->bbox.AddPoint(p11);

            if (level < (subdivLevelCount - 1))
            {
                //Calculating max absolute height error between neighbour lods.
                //Choosing from five averaged heights per quad: four on middle of edges and one on diagonal
                // +---*---+
                // | \     |
                // |  \    |
                // *   *   *
                // |    \  |
                // |     \ |
                // +---*---+

                //Accurate height values from next subdivide level (more detailed LOD)
                Vector3 p0[5] = {
                    heightmap->GetPoint(x_, y0, bbox),
                    heightmap->GetPoint(x0, y_, bbox),
                    heightmap->GetPoint(x_, y_, bbox),
                    heightmap->GetPoint(x1, y_, bbox),
                    heightmap->GetPoint(x_, y1, bbox),
                };
                //Averaged height values from current level (less detailed LOD)
                float32 h1[5] = {
                    (p00.z + p10.z) / 2.f,
                    (p00.z + p01.z) / 2.f,
                    (p00.z + p11.z) / 2.f,
                    (p10.z + p11.z) / 2.f,
                    (p01.z + p11.z) / 2.f,
                };

                //Calculate max error for quad
                for (int32 i = 0; i < 5; ++i)
                {
                    float32 error = p0[i].z - h1[i];
                    if (Abs(patch->maxError) < Abs(error))
                    {
                        patch->maxError = error;
                        patch->positionOfMaxError = p0[i];
                    }
                }
            }
        }
    }

    uint32 x2 = x << 1;
    uint32 y2 = y << 1;

    //UpdatePatchInfo can modify 'maxError' and 'bbox' of parentPatch
    UpdatePatchInfo(level + 1, x2 + 0, y2 + 0, patch, updateRect);
    UpdatePatchInfo(level + 1, x2 + 1, y2 + 0, patch, updateRect);
    UpdatePatchInfo(level + 1, x2 + 0, y2 + 1, patch, updateRect);
    UpdatePatchInfo(level + 1, x2 + 1, y2 + 1, patch, updateRect);

    patch->radius = Distance(patch->bbox.GetCenter(), patch->bbox.max);

    if (parentPatch)
    {
        if (Abs(parentPatch->maxError) < Abs(patch->maxError))
        {
            parentPatch->maxError = patch->maxError;
            parentPatch->positionOfMaxError = patch->positionOfMaxError;
        }

        parentPatch->bbox.AddAABBox(patch->bbox);
    }
}

void LandscapeSubdivision::SubdividePatch(uint32 level, uint32 x, uint32 y, uint8 clippingFlags, float32 heightError0, float32 radiusError0)
{
    SubdivisionLevelInfo& levelInfo = subdivLevelInfoArray[level];
    uint32 offset = levelInfo.offset + (y << level) + x;
    PatchQuadInfo* patch = &patchQuadArray[offset];
    SubdivisionPatchInfo* subdivPatchInfo = &subdivPatchArray[offset];
    subdivPatchInfo->lastUpdateID = updateID;

    // Calculate patch bounding box
    Frustum::eFrustumResult frustumRes = Frustum::EFR_INSIDE;

    if (clippingFlags)
        frustumRes = frustum->Classify(patch->bbox, clippingFlags, subdivPatchInfo->startClipPlane);

    if (frustumRes == Frustum::EFR_OUTSIDE)
    {
        subdivPatchInfo->subdivisionState = SubdivisionPatchInfo::CLIPPED;
        return;
    }

    ////////////////////////////////////////////////////////////////////////////////////

    //Metrics errors we calculate as projection on screen
    //
    //                     /              |
    //                  /                 ^ - error in world-space
    //               /                    |
    //            /                       |
    //         /|                         |
    //      /   ^ - error in screen-space |
    //   /      |                         |
    //  0---------------------------------D-------- frustum axis
    //          ^                         ^
    //      near plane            error position plane
    // plane size 1.0 a-priory   plane size let it be 'H'
    //
    // H = D * tg(fov/2), were D - is distance to error position
    // To find error size on near plane we need just error size divide by 'H'
    // So, screen space error = error / (D * tg(fov/2))
    // tg(fov/2) calculating one per-frame, see 'tanFovY' in PrepareSubdivision()

    float32 distance = Distance(cameraPos, patch->positionOfMaxError);
    float32 heightError = Abs(patch->maxError) / (distance * tanFovY);

    Vector3 patchOrigin = patch->bbox.GetCenter();
    float32 patchDistance = Distance(cameraPos, patchOrigin);
    float32 radiusError = patch->radius / (patchDistance * tanFovY);

    if ((level < subdivLevelCount - 1) && ((maxPatchRadiusError <= radiusError) || (maxHeightError <= heightError) || (maxAbsoluteHeightError < Abs(patch->maxError)) || (minSubdivLevel > level) || forceMaxSubdiv))
    {
        subdivPatchInfo->subdivisionState = SubdivisionPatchInfo::SUBDIVIDED;

        uint32 x2 = x << 1;
        uint32 y2 = y << 1;

        SubdividePatch(level + 1, x2 + 0, y2 + 0, clippingFlags, heightError, radiusError);
        SubdividePatch(level + 1, x2 + 1, y2 + 0, clippingFlags, heightError, radiusError);
        SubdividePatch(level + 1, x2 + 0, y2 + 1, clippingFlags, heightError, radiusError);
        SubdividePatch(level + 1, x2 + 1, y2 + 1, clippingFlags, heightError, radiusError);
    }
    else
    {
        if (calculateMorph)
        {
            float32 radiusError0Rel = Max(radiusError0, maxPatchRadiusError) / maxPatchRadiusError;
            float32 radiusErrorRel = Min(radiusError, maxPatchRadiusError) / maxPatchRadiusError;

            float32 heightError0Rel = Max(heightError0, maxHeightError) / maxHeightError;
            float32 heightErrorRel = Min(heightError, maxHeightError) / maxHeightError;

            float32 error0Delta = Max(radiusError0Rel, heightError0Rel) - 1.f;
            float32 errorDelta = 1.f - Max(radiusErrorRel, heightErrorRel);

            subdivPatchInfo->subdivMorph = 1.f - errorDelta / (error0Delta + errorDelta);
        }

        subdivPatchInfo->subdivisionState = SubdivisionPatchInfo::TERMINATED;

        terminatedPatchesCount++;
    }
}

const LandscapeSubdivision::SubdivisionPatchInfo* LandscapeSubdivision::GetTerminatedPatchInfo(uint32 level, uint32 x, uint32 y, uint32& patchLevel) const
{
    uint32 levelSize = 1 << level;
    do
    {
        if (x >= levelSize || y >= levelSize)
        {
            return nullptr;
        }

        const LandscapeSubdivision::SubdivisionPatchInfo& patchInfo = GetPatchInfo(level, x, y);
        if (patchInfo.lastUpdateID == updateID)
        {
            if (patchInfo.subdivisionState == SubdivisionPatchInfo::TERMINATED)
            {
                patchLevel = level;
                return &patchInfo;
            }
            else
            {
                return nullptr;
            }
        }

        --level;
        levelSize /= 2;
        x /= 2;
        y /= 2;
    } while (level != uint32(-1));

    return nullptr;
}

void LandscapeSubdivision::BuildSubdivision(Heightmap* _heightmap, const AABBox3& _bbox, uint32 _patchSizeQuads, uint32 minSubdivideLevel, bool _calculateMorph)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    ReleaseInternalData();

    DVASSERT(_heightmap);

    heightmap = SafeRetain(_heightmap);
    bbox = _bbox;
    minSubdivLevel = minSubdivideLevel;
    patchSizeQuads = _patchSizeQuads;
    calculateMorph = _calculateMorph;

    subdivLevelCount = FastLog2(heightmap->Size() / patchSizeQuads) + 1;
    subdivLevelInfoArray.resize(subdivLevelCount);
    subdivPatchCount = 0;

    uint32 size = 1;
    for (uint32 k = 0; k < subdivLevelCount; ++k)
    {
        subdivLevelInfoArray[k].offset = subdivPatchCount;
        subdivLevelInfoArray[k].size = size;
        subdivPatchCount += size * size;
        size *= 2;
    }

    subdivPatchArray.resize(subdivPatchCount);
    patchQuadArray.resize(subdivPatchCount);

    UpdatePatchInfo(0, 0, 0, nullptr, Rect2i(0, 0, -1, -1));
}
}
