#pragma once

#include <Base/Any.h>
#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "Base/IntrospectionBase.h"
#include "MemoryManager/MemoryProfiler.h"

namespace DAVA
{
class Frustum;
class Heightmap;
class Camera;

class LandscapeSubdivision : public InspBase
{
    DAVA_ENABLE_CLASS_ALLOCATION_TRACKING(ALLOC_POOL_LANDSCAPE)

public:
    LandscapeSubdivision();
    ~LandscapeSubdivision();

    struct SubdivisionPatchInfo
    {
        enum
        {
            CLIPPED = 1,
            SUBDIVIDED = 2,
            TERMINATED = 3,
        };

        uint32 lastUpdateID = 0;
        float32 subdivMorph = 0.f;
        uint8 subdivisionState = CLIPPED;
        uint8 startClipPlane = 0;
    };

    struct SubdivisionLevelInfo
    {
        uint32 offset;
        uint32 size;
    };

    struct SubdivisionMetrics : public InspBase
    {
        float32 normalFov = 70.f;
        float32 zoomFov = 6.5f;

        float32 normalMaxHeightError = 0.014f;
        float32 normalMaxPatchRadiusError = 0.45f;
        float32 normalMaxAbsoluteHeightError = 3.f;

        float32 zoomMaxHeightError = 0.03f;
        float32 zoomMaxPatchRadiusError = 0.9f;
        float32 zoomMaxAbsoluteHeightError = 3.f;

        bool operator==(const SubdivisionMetrics& other) const;

        DAVA_VIRTUAL_REFLECTION(SubdivisionMetrics, InspBase);
    };

    void BuildSubdivision(Heightmap* heightmap, const AABBox3& bbox, uint32 patchSizeQuads, uint32 minSubdivideLevel, bool calculateMorph);
    void PrepareSubdivision(Camera* camera, const Matrix4* worldTransform);
    void ReleaseInternalData();

    const SubdivisionLevelInfo& GetLevelInfo(uint32 level) const;
    const SubdivisionPatchInfo& GetPatchInfo(uint32 level, uint32 x, uint32 y) const;
    const SubdivisionPatchInfo* GetTerminatedPatchInfo(uint32 level, uint32 x, uint32 y, uint32& patchLevel) const;
    SubdivisionMetrics& GetMetrics();

    uint32 GetLevelCount() const;
    uint32 GetPatchCount() const;
    uint32 GetTerminatedPatchesCount() const;

    void UpdatePatchInfo(const Rect2i& heighmapRect);
    void SetForceMaxSubdivision(bool forceSubdivide);

private:
    struct PatchQuadInfo
    {
        AABBox3 bbox;
        Vector3 positionOfMaxError;
        float32 maxError;
        float32 radius;
    };

    void UpdatePatchInfo(uint32 level, uint32 x, uint32 y, PatchQuadInfo* parentPatch, const Rect2i& updateRect);
    void SubdividePatch(uint32 level, uint32 x, uint32 y, uint8 clippingFlags, float32 heightError0, float32 radiusError0);

    const PatchQuadInfo& GetPatchQuadInfo(uint32 level, uint32 x, uint32 y) const;

    Vector<SubdivisionLevelInfo> subdivLevelInfoArray;
    Vector<PatchQuadInfo> patchQuadArray;
    Vector<SubdivisionPatchInfo> subdivPatchArray;
    uint32 terminatedPatchesCount = 0;

    uint32 minSubdivLevel = 0;
    uint32 subdivLevelCount = 0;
    uint32 subdivPatchCount = 0;
    uint32 patchSizeQuads = 8;
    uint32 updateID = 0;

    SubdivisionMetrics metrics;

    float32 maxHeightError = 0.f;
    float32 maxPatchRadiusError = 0.f;
    float32 maxAbsoluteHeightError = 0.f;

    Vector3 cameraPos;
    float32 tanFovY = 0.f;

    Frustum* frustum = nullptr;
    Heightmap* heightmap = nullptr;

    AABBox3 bbox;

    bool calculateMorph = true;
    bool forceMaxSubdiv = false;

    friend class LandscapeSystem;

    DAVA_VIRTUAL_REFLECTION(LandscapeSubdivision, InspBase);
};

inline const LandscapeSubdivision::SubdivisionLevelInfo& LandscapeSubdivision::GetLevelInfo(uint32 level) const
{
    DVASSERT(level < subdivLevelInfoArray.size());
    return subdivLevelInfoArray[level];
}

inline const LandscapeSubdivision::SubdivisionPatchInfo& LandscapeSubdivision::GetPatchInfo(uint32 level, uint32 x, uint32 y) const
{
    const SubdivisionLevelInfo& levelInfo = GetLevelInfo(level);
    DVASSERT(x < levelInfo.size && y < levelInfo.size);
    return subdivPatchArray[levelInfo.offset + (y << level) + x];
}

inline const LandscapeSubdivision::PatchQuadInfo& LandscapeSubdivision::GetPatchQuadInfo(uint32 level, uint32 x, uint32 y) const
{
    const SubdivisionLevelInfo& levelInfo = GetLevelInfo(level);
    DVASSERT(x < levelInfo.size && y < levelInfo.size);
    return patchQuadArray[levelInfo.offset + (y << level) + x];
}

inline void LandscapeSubdivision::SetForceMaxSubdivision(bool forceSubdivide)
{
    forceMaxSubdiv = forceSubdivide;
}

inline uint32 LandscapeSubdivision::GetLevelCount() const
{
    return subdivLevelCount;
}

inline uint32 LandscapeSubdivision::GetPatchCount() const
{
    return subdivPatchCount;
}

inline uint32 LandscapeSubdivision::GetTerminatedPatchesCount() const
{
    return terminatedPatchesCount;
}

inline LandscapeSubdivision::SubdivisionMetrics& LandscapeSubdivision::GetMetrics()
{
    return metrics;
}

template <>
bool AnyCompare<LandscapeSubdivision::SubdivisionMetrics>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2);
extern template struct AnyCompare<LandscapeSubdivision::SubdivisionMetrics>;
}
