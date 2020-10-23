#ifndef __DAVAENGINE_STATIC_OCCLUSION__
#define __DAVAENGINE_STATIC_OCCLUSION__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/BaseMath.h"
#include "Render/RenderBase.h"
#include "Render/Texture.h"

namespace DAVA
{
class Camera;
class StaticOcclusionRenderPass;
class RenderObject;
class RenderHierarchy;
class RenderBatch;
class RenderSystem;
class Scene;
class Sprite;
class Landscape;

class StaticOcclusionData
{
public:
    StaticOcclusionData();
    ~StaticOcclusionData();

    void Init(uint32 sizeX, uint32 sizeY, uint32 sizeZ, uint32 objectCount, const AABBox3& bbox, const float32* _cellHeightOffset);
    void EnableVisibilityForObject(uint32 blockIndex, uint32 objectIndex);
    void DisableVisibilityForObject(uint32 blockIndex, uint32 objectIndex);

    bool IsObjectVisibleFromBlock(uint32 blockIndex, uint32 objectIndex) const;

    uint32* GetBlockVisibilityData(uint32 blockIndex);
    StaticOcclusionData& operator=(const StaticOcclusionData& other);

    void SetData(const uint32* _data, uint32 dataSize);
    const uint32* GetData() const;

public:
    AABBox3 bbox;
    uint32 sizeX = 0;
    uint32 sizeY = 0;
    uint32 sizeZ = 0;
    uint32 blockCount = 0;
    uint32 objectCount = 0;
    float32* cellHeightOffset = nullptr;

private:
    Vector<uint32> dataHolder;
};

struct StaticOcclusionFrameResult
{
    uint32 blockIndex = 0;
    rhi::HQueryBuffer queryBuffer = rhi::HQueryBuffer(rhi::InvalidHandle);
    Vector<RenderObject*> frameRequests;
    DAVA::UnorderedMap<uint16, int32> samplesPassed; // First - object's static occlusion index. Second - pixels drawn for this object in current block.
};

class StaticOcclusion
{
public:
    StaticOcclusion();
    ~StaticOcclusion();

    void StartBuildOcclusion(StaticOcclusionData* currentData, RenderSystem* renderSystem, Landscape* landscape, uint32 occlusionPixelThreshold, uint32 occlusionPixelThresholdForSpeedtree);
    bool ProcessBlock(); // returns true if finished building
    void AdvanceToNextBlock();

    uint32 GetCurrentStepsCount();
    uint32 GetTotalStepsCount();

    const String& GetInfoMessage() const;

private:
    AABBox3 GetCellBox(uint32 x, uint32 y, uint32 z);

    void MarkQueriesAsCompletedForObjectInBlock(uint16 objectIndex, uint32 blockIndex);
    bool ProcessRecorderQueries();

    struct RenderPassCameraConfig
    {
        Vector3 position;
        Vector3 left;
        Vector3 up;
        Vector3 direction;
        uint32 side = 0;
        uint32 blockIndex = 0;
    };

    struct Statistics
    {
        uint64 blockProcessingTime = 0;
        double buildDuration = 0.0;
        uint64 buildStartTime = 0;
        uint64 totalRenderPasses = 0;
    } stats; //-V730_NOINIT

    void UpdateInfoString();
    void BuildRenderPassConfigsForCurrentBlock();
    bool RenderCurrentBlock(); // returns true, if all passes for block completed
    bool PerformRender(const RenderPassCameraConfig&);

private:
    std::array<Camera*, 6> cameras;
    StaticOcclusionRenderPass* staticOcclusionRenderPass = nullptr;
    StaticOcclusionData* currentData = nullptr;
    RenderSystem* renderSystem = nullptr;
    Landscape* landscape = nullptr;
    float32* cellHeightOffset = nullptr;
    Vector<StaticOcclusionFrameResult> occlusionFrameResults;
    Vector<RenderPassCameraConfig> renderPassConfigs;
    String lastInfoMessage;
    AABBox3 occlusionAreaRect;
    uint32 xBlockCount = 0;
    uint32 yBlockCount = 0;
    uint32 zBlockCount = 0;
    uint32 currentFrameX = 0;
    uint32 currentFrameY = 0;
    uint32 currentFrameZ = 0;
    uint32 occlusionPixelThreshold = 0;
    uint32 occlusionPixelThresholdForSpeedtree = 0;
};
};

#endif //__DAVAENGINE_STATIC_OCCLUSION__
