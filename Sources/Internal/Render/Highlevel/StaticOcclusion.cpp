#include "Render/RenderHelper.h"
#include "Render/Highlevel/StaticOcclusion.h"
#include "Render/Highlevel/StaticOcclusionRenderPass.h"
#include "Render/Highlevel/RenderBatchArray.h"
#include "Render/Highlevel/Camera.h"
#include "Render/Image/Image.h"
#include "Utils/StringFormat.h"
#include "Utils/Random.h"
#include "Time/SystemTimer.h"
#include "Render/Highlevel/Landscape.h"
#include "Render/Image/ImageSystem.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"

namespace DAVA
{
StaticOcclusion::StaticOcclusion()
{
    for (uint32 k = 0; k < 6; ++k)
    {
        cameras[k] = new Camera();
        cameras[k]->SetupPerspective(95.0f, 1.0f, 1.0f, 2500.0f); //aspect of one is anyway required to avoid side occlusion errors
    }
}

StaticOcclusion::~StaticOcclusion()
{
    for (uint32 k = 0; k < 6; ++k)
    {
        SafeRelease(cameras[k]);
    }
    SafeDelete(staticOcclusionRenderPass);
}

void StaticOcclusion::StartBuildOcclusion(StaticOcclusionData* _currentData, RenderSystem* _renderSystem, Landscape* _landscape, uint32 _occlusionPixelThreshold, uint32 _occlusionPixelThresholdForSpeedtree)
{
    lastInfoMessage = "Preparing to build static occlusion...";
    staticOcclusionRenderPass = new StaticOcclusionRenderPass(PASS_FORWARD);

    currentData = _currentData;
    occlusionAreaRect = currentData->bbox;
    cellHeightOffset = currentData->cellHeightOffset;
    xBlockCount = currentData->sizeX;
    yBlockCount = currentData->sizeY;
    zBlockCount = currentData->sizeZ;

    stats.buildStartTime = SystemTimer::GetNs();
    stats.blockProcessingTime = stats.buildStartTime;
    stats.buildDuration = 0.0;
    stats.totalRenderPasses = 0;

    currentFrameX = -1; // we increasing this, before rendering, so we will start from zero
    currentFrameY = 0;
    currentFrameZ = 0;

    renderSystem = _renderSystem;
    landscape = _landscape;

    occlusionPixelThreshold = _occlusionPixelThreshold;
    occlusionPixelThresholdForSpeedtree = _occlusionPixelThresholdForSpeedtree;
}

AABBox3 StaticOcclusion::GetCellBox(uint32 x, uint32 y, uint32 z)
{
    Vector3 size = occlusionAreaRect.GetSize();

    size.x /= xBlockCount;
    size.y /= yBlockCount;
    size.z /= zBlockCount;

    Vector3 min(occlusionAreaRect.min.x + x * size.x,
                occlusionAreaRect.min.y + y * size.y,
                occlusionAreaRect.min.z + z * size.z);
    if (cellHeightOffset)
    {
        min.z += cellHeightOffset[x + y * xBlockCount];
    }
    AABBox3 blockBBox(min, Vector3(min.x + size.x, min.y + size.y, min.z + size.z));
    return blockBBox;
}

void StaticOcclusion::AdvanceToNextBlock()
{
    currentFrameX++;
    if (currentFrameX >= xBlockCount)
    {
        currentFrameX = 0;
        currentFrameY++;
        if (currentFrameY >= yBlockCount)
        {
            currentFrameY = 0;
            currentFrameZ++;
        }
    }
}

bool StaticOcclusion::ProcessBlock()
{
    if (!ProcessRecorderQueries())
    {
        RenderCurrentBlock();
        UpdateInfoString();
        return false;
    }

    if (renderPassConfigs.empty())
    {
        AdvanceToNextBlock();

        auto currentTime = SystemTimer::GetNs();
        stats.buildDuration += static_cast<double>(currentTime - stats.blockProcessingTime) / 1e+9;
        stats.blockProcessingTime = currentTime;

        if (currentFrameZ >= zBlockCount) // all blocks processed
        {
            UpdateInfoString();
            return true;
        }

        BuildRenderPassConfigsForCurrentBlock();
    }

    RenderCurrentBlock();
    UpdateInfoString();

    return false;
}

uint32 StaticOcclusion::GetCurrentStepsCount()
{
    return currentFrameX + (currentFrameY * xBlockCount) + (currentFrameZ * xBlockCount * yBlockCount);
}

uint32 StaticOcclusion::GetTotalStepsCount()
{
    return xBlockCount * yBlockCount * zBlockCount;
}

const String& StaticOcclusion::GetInfoMessage() const
{
    return lastInfoMessage;
}

void StaticOcclusion::BuildRenderPassConfigsForCurrentBlock()
{
    const uint32 stepCount = 10;

    const Vector3 viewDirections[6] =
    {
      Vector3(1.0f, 0.0f, 0.0f), //  x 0
      Vector3(0.0f, 1.0f, 0.0f), //  y 1
      Vector3(-1.0f, 0.0f, 0.0f), // -x 2
      Vector3(0.0f, -1.0f, 0.0f), // -y 3
      Vector3(0.0f, 0.0f, 1.0f), // +z 4
      Vector3(0.0f, 0.0f, -1.0f), // -z 5
    };

    const uint32 effectiveSideCount[6] = { 3, 3, 3, 3, 1, 1 };

    const uint32 effectiveSides[6][3] =
    {
      { 0, 1, 3 },
      { 1, 0, 2 },
      { 2, 1, 3 },
      { 3, 0, 2 },
      { 4, 4, 4 },
      { 5, 5, 5 },
    };

    uint32 blockIndex = currentFrameX + currentFrameY * xBlockCount + currentFrameZ * xBlockCount * yBlockCount;
    AABBox3 cellBox = GetCellBox(currentFrameX, currentFrameY, currentFrameZ);
    Vector3 stepSize = cellBox.GetSize();
    stepSize /= float32(stepCount);

    DVASSERT(occlusionFrameResults.size() == 0); // previous results are processed - at least for now

    for (uint32 side = 0; side < 6; ++side)
    {
        Vector3 startPosition, directionX, directionY;
        if (side == 0) // +x
        {
            startPosition = Vector3(cellBox.max.x, cellBox.min.y, cellBox.min.z);
            directionX = Vector3(0.0f, 1.0f, 0.0f);
            directionY = Vector3(0.0f, 0.0f, 1.0f);
        }
        else if (side == 2) // -x
        {
            startPosition = Vector3(cellBox.min.x, cellBox.min.y, cellBox.min.z);
            directionX = Vector3(0.0f, 1.0f, 0.0f);
            directionY = Vector3(0.0f, 0.0f, 1.0f);
        }
        else if (side == 1) // +y
        {
            startPosition = Vector3(cellBox.min.x, cellBox.max.y, cellBox.min.z);
            directionX = Vector3(1.0f, 0.0f, 0.0f);
            directionY = Vector3(0.0f, 0.0f, 1.0f);
        }
        else if (side == 3) // -y
        {
            startPosition = Vector3(cellBox.min.x, cellBox.min.y, cellBox.min.z);
            directionX = Vector3(1.0f, 0.0f, 0.0f);
            directionY = Vector3(0.0f, 0.0f, 1.0f);
        }
        else if (side == 4) // +z
        {
            startPosition = Vector3(cellBox.min.x, cellBox.min.y, cellBox.max.z);
            directionX = Vector3(1.0f, 0.0f, 0.0f);
            directionY = Vector3(0.0f, 1.0f, 0.0f);
        }
        else if (side == 5) // -z
        {
            startPosition = Vector3(cellBox.min.x, cellBox.min.y, cellBox.min.z);
            directionX = Vector3(1.0f, 0.0f, 0.0f);
            directionY = Vector3(0.0f, 1.0f, 0.0f);
        }

        for (uint32 realSideIndex = 0; realSideIndex < effectiveSideCount[side]; ++realSideIndex)
        {
            for (uint32 stepX = 0; stepX <= stepCount; ++stepX)
            {
                for (uint32 stepY = 0; stepY <= stepCount; ++stepY)
                {
                    Vector3 renderPosition = startPosition + directionX * float32(stepX) * stepSize + directionY * float32(stepY) * stepSize;

                    if (landscape)
                    {
                        Vector3 pointOnLandscape;
                        if (landscape->PlacePoint(renderPosition, pointOnLandscape))
                        {
                            if (renderPosition.z < pointOnLandscape.z)
                                continue;
                        }
                    }

                    RenderPassCameraConfig config;
                    config.blockIndex = blockIndex;
                    config.side = side;
                    config.position = renderPosition;
                    config.direction = viewDirections[effectiveSides[side][realSideIndex]];
                    if (effectiveSides[side][realSideIndex] == 4 || effectiveSides[side][realSideIndex] == 5)
                    {
                        config.up = Vector3(0.0f, 1.0f, 0.0f);
                        config.left = Vector3(1.0f, 0.0f, 0.0f);
                    }
                    else
                    {
                        config.up = Vector3(0.0f, 0.0f, 1.0f);
                        config.left = Vector3(1.0f, 0.0f, 0.0f);
                    }
                    renderPassConfigs.push_back(config);
                }
            }
        }
    }

    stats.totalRenderPasses = renderPassConfigs.size();
}

bool StaticOcclusion::PerformRender(const RenderPassCameraConfig& rpc)
{
    Camera* camera = cameras[rpc.side];
    camera->SetPosition(rpc.position);
    camera->SetLeft(rpc.left);
    camera->SetUp(rpc.up);
    camera->SetDirection(rpc.direction);

    occlusionFrameResults.emplace_back();
    StaticOcclusionFrameResult& res = occlusionFrameResults.back();
    staticOcclusionRenderPass->DrawOcclusionFrame(renderSystem, camera, res, *currentData, rpc.blockIndex);
    if (res.queryBuffer == rhi::InvalidHandle)
    {
        occlusionFrameResults.pop_back();
        return false;
    }

    return true;
}

bool StaticOcclusion::RenderCurrentBlock()
{
    uint64 renders = 0;

#if (SAVE_OCCLUSION_IMAGES)
    uint64 maxRenders = 1;
#else
    uint64 maxRenders = 48;
#endif

    Random* random = GetEngineContext()->random;
    while ((renders < maxRenders) && !renderPassConfigs.empty())
    {
        auto i = renderPassConfigs.begin();
        std::advance(i, random->Rand() % renderPassConfigs.size());
        PerformRender(*i);
        renderPassConfigs.erase(i);
        ++renders;
    }

    return renderPassConfigs.empty();
}

void StaticOcclusion::MarkQueriesAsCompletedForObjectInBlock(uint16 objectIndex, uint32 blockIndex)
{
    for (auto& ofr : occlusionFrameResults)
    {
        if (ofr.blockIndex == blockIndex)
        {
            for (auto& req : ofr.frameRequests)
            {
                if ((req != nullptr) && (req->GetStaticOcclusionIndex() == objectIndex))
                    req = nullptr;
            }
        }
    }
}

bool StaticOcclusion::ProcessRecorderQueries()
{
    auto fr = occlusionFrameResults.begin();

    while (fr != occlusionFrameResults.end())
    {
        uint32 processedRequests = 0;
        uint32 index = 0;
        for (auto& req : fr->frameRequests)
        {
            if (req == nullptr)
            {
                ++processedRequests;
                ++index;
                continue;
            }

            DVASSERT(req->GetStaticOcclusionIndex() != INVALID_STATIC_OCCLUSION_INDEX);

            if (rhi::QueryIsReady(fr->queryBuffer, index))
            {
                int32& samplesPassed = fr->samplesPassed[req->GetStaticOcclusionIndex()];
                samplesPassed += rhi::QueryValue(fr->queryBuffer, index);

                uint32 threshold = req->GetType() != RenderObject::TYPE_SPEED_TREE ?
                occlusionPixelThreshold :
                occlusionPixelThresholdForSpeedtree;

                if (static_cast<uint32>(samplesPassed) > threshold)
                {
                    bool alreadyVisible = currentData->IsObjectVisibleFromBlock(fr->blockIndex, req->GetStaticOcclusionIndex());
                    DVASSERT(!alreadyVisible);
                    currentData->EnableVisibilityForObject(fr->blockIndex, req->GetStaticOcclusionIndex());
                    MarkQueriesAsCompletedForObjectInBlock(req->GetStaticOcclusionIndex(), fr->blockIndex);
                }
                ++processedRequests;
                req = nullptr;
            }

            ++index;
        }

        if (processedRequests == static_cast<uint32>(fr->frameRequests.size()))
        {
            rhi::DeleteQueryBuffer(fr->queryBuffer, true);
            fr = occlusionFrameResults.erase(fr);
        }
        else
        {
            ++fr;
        }
    }

    return occlusionFrameResults.empty();
}

// helper function, see implementation below
namespace helper
{
String FormatTime(double fSeconds);
}

void StaticOcclusion::UpdateInfoString()
{
    auto totalBlocks = xBlockCount * yBlockCount * zBlockCount;
    auto blockIndex = currentFrameX + currentFrameY * xBlockCount + currentFrameZ * xBlockCount * yBlockCount;
    if (blockIndex >= totalBlocks)
    {
        lastInfoMessage = Format("Completed. Total time spent: %s", helper::FormatTime(stats.buildDuration).c_str());
    }
    else
    {
        float32 fTotalRenders = static_cast<float32>(stats.totalRenderPasses);
        float32 fRemainingRenders = static_cast<float32>(renderPassConfigs.size());
        float32 rendersCompleted = (stats.totalRenderPasses == 0) ? 1.0f : (1.0f - fRemainingRenders / fTotalRenders);

        auto averageTime = (blockIndex == 0) ? 0.0 : (stats.buildDuration / static_cast<double>(blockIndex));
        auto remainingBlocks = totalBlocks - blockIndex;
        auto remainingTime = static_cast<double>(remainingBlocks) * averageTime;
        lastInfoMessage = Format("Processing block: %u from %u (%d%% completed) \n\nTotal time spent: %s\nEstimated remaining time: %s",
                                 blockIndex + 1, totalBlocks, static_cast<int>(100.0f * rendersCompleted),
                                 helper::FormatTime(stats.buildDuration).c_str(), helper::FormatTime(remainingTime).c_str());
    }
}

StaticOcclusionData::StaticOcclusionData()
    : sizeX(5)
    , sizeY(5)
    , sizeZ(2)
    , blockCount(0)
    , objectCount(0)
    , cellHeightOffset(0)
{
}

StaticOcclusionData::~StaticOcclusionData()
{
    SafeDeleteArray(cellHeightOffset);
}

StaticOcclusionData& StaticOcclusionData::operator=(const StaticOcclusionData& other)
{
    sizeX = other.sizeX;
    sizeY = other.sizeY;
    sizeZ = other.sizeZ;
    objectCount = other.objectCount;
    blockCount = other.blockCount;
    bbox = other.bbox;
    dataHolder = other.dataHolder;

    SafeDeleteArray(cellHeightOffset);
    if (other.cellHeightOffset)
    {
        cellHeightOffset = new float32[sizeX * sizeY];
        memcpy(cellHeightOffset, other.cellHeightOffset, sizeof(float32) * sizeX * sizeY);
    }

    return *this;
}

void StaticOcclusionData::Init(uint32 _sizeX, uint32 _sizeY, uint32 _sizeZ, uint32 _objectCount,
                               const AABBox3& _bbox, const float32* _cellHeightOffset)
{
    SafeDeleteArray(cellHeightOffset);

    objectCount = _objectCount;
    sizeX = _sizeX;
    sizeY = _sizeY;
    sizeZ = _sizeZ;
    blockCount = sizeX * sizeY * sizeZ;
    bbox = _bbox;

    objectCount += (32 - objectCount & 31);

    auto numElements = blockCount * objectCount / 32;
    dataHolder.resize(numElements);
    std::fill(dataHolder.begin(), dataHolder.end(), 0);

    if (_cellHeightOffset)
    {
        cellHeightOffset = new float32[sizeX * sizeY];
        memcpy(cellHeightOffset, _cellHeightOffset, sizeof(float32) * sizeX * sizeY);
    }
}

bool StaticOcclusionData::IsObjectVisibleFromBlock(uint32 blockIndex, uint32 objectIndex) const
{
    auto objIndex = 1 << (objectIndex & 31);
    auto index = (blockIndex * objectCount / 32) + (objectIndex / 32);
    DVASSERT(index < dataHolder.size());
    return (dataHolder[index] & objIndex) != 0;
}

void StaticOcclusionData::EnableVisibilityForObject(uint32 blockIndex, uint32 objectIndex)
{
    auto index = (blockIndex * objectCount / 32) + (objectIndex / 32);
    DVASSERT(index < dataHolder.size());
    dataHolder[index] |= 1 << (objectIndex & 31);
}

void StaticOcclusionData::DisableVisibilityForObject(uint32 blockIndex, uint32 objectIndex)
{
    auto index = (blockIndex * objectCount / 32) + (objectIndex / 32);
    DVASSERT(index < dataHolder.size());
    dataHolder[index] &= ~(1 << (objectIndex & 31));
}

uint32* StaticOcclusionData::GetBlockVisibilityData(uint32 blockIndex)
{
    auto index = blockIndex * objectCount / 32;
    DVASSERT(index < dataHolder.size());
    return dataHolder.data() + index;
}

const uint32* StaticOcclusionData::GetData() const
{
    return dataHolder.data();
}

void StaticOcclusionData::SetData(const uint32* _data, uint32 dataSize)
{
    auto elements = dataSize / sizeof(uint32);
    dataHolder.resize(elements);
    std::copy(_data, _data + elements, dataHolder.begin());
}

namespace helper
{
String FormatTime(double fSeconds)
{
    uint64 seconds = static_cast<uint64>(fSeconds);
    uint64 minutes = seconds / 60;
    seconds -= minutes * 60;
    uint64 hours = minutes / 60;
    minutes -= hours * 60;
    char buffer[1024] = {};
    sprintf(buffer, "%02llu:%02llu:%02llu", hours, minutes, seconds);
    return String(buffer);
}
}
};
