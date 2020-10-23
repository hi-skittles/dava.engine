#include "VisibilityQueryResults.h"
#include "Debug/DVAssert.h"
#include "RHI/rhi_Public.h"

namespace DAVA
{
namespace VisibilityQueryResultsDetails
{
using FrameQueryBuffers = Vector<rhi::HQueryBuffer>;
Vector<rhi::HQueryBuffer> queryBufferPool;
Vector<FrameQueryBuffers> pendingQueryBuffer;
FrameQueryBuffers currentQueryBuffers;

uint32 frameVisibilityResults[VisibilityQueryResults::QUERY_INDEX_COUNT] = {};

bool QueryBuffersIsReady(const FrameQueryBuffers& buffers)
{
    for (rhi::HQueryBuffer handle : buffers)
    {
        if (!rhi::QueryBufferIsReady(handle))
            return false;
    }

    return true;
}

} //ns Details

namespace VisibilityQueryResults
{
using namespace VisibilityQueryResultsDetails;

rhi::HQueryBuffer GetQueryBuffer()
{
    rhi::HQueryBuffer buffer;
#ifdef __DAVAENGINE_RENDERSTATS__
    if (queryBufferPool.empty())
    {
        buffer = rhi::CreateQueryBuffer(QUERY_INDEX_COUNT);
    }
    else
    {
        buffer = queryBufferPool.back();
        queryBufferPool.pop_back();
    }
#endif

    if (buffer.IsValid())
        currentQueryBuffers.push_back(buffer);

    return buffer;
}

uint32 GetResult(eQueryIndex index)
{
    return frameVisibilityResults[index];
}

const FastName& GetQueryIndexName(eQueryIndex index)
{
    static const FastName queryIndexNames[QUERY_INDEX_COUNT] =
    {
      FastName("OpaqueRenderLayer"),
      FastName("AfterOpaqueRenderLayer"),
      FastName("AlphaTestLayer"),
      FastName("WaterLayer"),
      FastName("TransclucentRenderLayer"),
      FastName("AfterTransclucentRenderLayer"),
      FastName("ShadowVolumeRenderLayer"),
      FastName("VegetationRenderLayer"),
      FastName("DebugRenderLayer"),
      FastName("UI"),
      FastName("Alpha-blend"),
    };

    return queryIndexNames[index];
}

void EndFrame()
{
    DVASSERT(pendingQueryBuffer.size() < 128);

    while (!pendingQueryBuffer.empty() && QueryBuffersIsReady(pendingQueryBuffer.front()))
    {
        Memset(frameVisibilityResults, 0, sizeof(frameVisibilityResults));
        for (rhi::HQueryBuffer h : pendingQueryBuffer.front())
        {
            for (uint32 i = 0; i < uint32(QUERY_INDEX_COUNT); ++i)
                frameVisibilityResults[i] += rhi::QueryValue(h, i);

            rhi::ResetQueryBuffer(h);
            queryBufferPool.push_back(h);
        }
        pendingQueryBuffer.erase(pendingQueryBuffer.begin());
    }

    pendingQueryBuffer.push_back(currentQueryBuffers);
    currentQueryBuffers.clear();
}

void Cleanup()
{
    for (FrameQueryBuffers& buffers : pendingQueryBuffer)
        queryBufferPool.insert(queryBufferPool.end(), buffers.begin(), buffers.end());

    pendingQueryBuffer.clear();

    for (rhi::HQueryBuffer h : queryBufferPool)
    {
        if (h.IsValid())
        {
            rhi::DeleteQueryBuffer(h);
        }
    }
}

} //ns VisibilityResults
} //ns DAVA