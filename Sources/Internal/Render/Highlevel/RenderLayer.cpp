#include "Render/Highlevel/RenderLayer.h"
#include "Render/Highlevel/RenderBatch.h"
#include "Render/Highlevel/RenderBatchArray.h"
#include "Render/Highlevel/Camera.h"
#include "Render/VisibilityQueryResults.h"
#include "Base/Radix/Radix.h"
#include "Debug/ProfilerGPU.h"
#include "Debug/ProfilerMarkerNames.h"

namespace DAVA
{
const FastName LAYER_NAME_OPAQUE("OpaqueRenderLayer");
const FastName LAYER_NAME_AFTER_OPAQUE("AfterOpaqueRenderLayer");
const FastName LAYER_NAME_ALPHA_TEST_LAYER("AlphaTestLayer");
const FastName LAYER_NAME_WATER("WaterLayer");
const FastName LAYER_NAME_TRANSLUCENT("TransclucentRenderLayer");
const FastName LAYER_NAME_AFTER_TRANSLUCENT("AfterTransclucentRenderLayer");
const FastName LAYER_NAME_SHADOW_VOLUME("ShadowVolumeRenderLayer");
const FastName LAYER_NAME_VEGETATION("VegetationRenderLayer");
const FastName LAYER_NAME_DEBUG_DRAW("DebugRenderLayer");

const FastName LAYER_NAMES[RenderLayer::RENDER_LAYER_ID_COUNT] =
{
  LAYER_NAME_OPAQUE,
  LAYER_NAME_AFTER_OPAQUE,
  LAYER_NAME_ALPHA_TEST_LAYER,
  LAYER_NAME_WATER,
  LAYER_NAME_TRANSLUCENT,
  LAYER_NAME_AFTER_TRANSLUCENT,
  LAYER_NAME_SHADOW_VOLUME,
  LAYER_NAME_VEGETATION,
  LAYER_NAME_DEBUG_DRAW
};

const uint32 RenderLayer::LAYER_SORTING_FLAGS_OPAQUE = RenderBatchArray::SORT_ENABLED | RenderBatchArray::SORT_BY_MATERIAL;
const uint32 RenderLayer::LAYER_SORTING_FLAGS_AFTER_OPAQUE = RenderBatchArray::SORT_ENABLED | RenderBatchArray::SORT_BY_MATERIAL;
const uint32 RenderLayer::LAYER_SORTING_FLAGS_ALPHA_TEST_LAYER = RenderBatchArray::SORT_ENABLED | RenderBatchArray::SORT_BY_MATERIAL;
const uint32 RenderLayer::LAYER_SORTING_FLAGS_WATER = 0;
const uint32 RenderLayer::LAYER_SORTING_FLAGS_TRANSLUCENT = RenderBatchArray::SORT_ENABLED | RenderBatchArray::SORT_BY_DISTANCE_BACK_TO_FRONT;
const uint32 RenderLayer::LAYER_SORTING_FLAGS_AFTER_TRANSLUCENT = RenderBatchArray::SORT_ENABLED | RenderBatchArray::SORT_BY_MATERIAL;
const uint32 RenderLayer::LAYER_SORTING_FLAGS_SHADOW_VOLUME = 0;
const uint32 RenderLayer::LAYER_SORTING_FLAGS_VEGETATION = 0;
const uint32 RenderLayer::LAYER_SORTING_FLAGS_DEBUG_DRAW = 0;

RenderLayer::RenderLayer(eRenderLayerID _id, uint32 sortingFlags)
    : layerID(_id)
    , sortFlags(sortingFlags)
{
}

RenderLayer::~RenderLayer()
{
}

const FastName& RenderLayer::GetLayerNameByID(eRenderLayerID layer)
{
    DVASSERT(layer >= 0 && layer < RENDER_LAYER_ID_COUNT);
    return LAYER_NAMES[layer];
}

RenderLayer::eRenderLayerID RenderLayer::GetLayerIDByName(const FastName& name)
{
    for (int32 id = 0; id < RENDER_LAYER_ID_COUNT; ++id)
    {
        if (LAYER_NAMES[id] == name)
        {
            return static_cast<eRenderLayerID>(id);
        }
    }
    return RENDER_LAYER_INVALID_ID;
}

void RenderLayer::Draw(Camera* camera, const RenderBatchArray& batchArray, rhi::HPacketList packetList)
{
    uint32 size = static_cast<uint32>(batchArray.GetRenderBatchCount());

    rhi::Packet packet;
    for (uint32 k = 0; k < size; ++k)
    {
        RenderBatch* batch = batchArray.Get(k);
        RenderObject* renderObject = batch->GetRenderObject();
        renderObject->BindDynamicParameters(camera, batch);
        NMaterial* mat = batch->GetMaterial();
        if (mat)
        {
            batch->BindGeometryData(packet);
            DVASSERT(packet.primitiveCount);
            mat->BindParams(packet);
            packet.debugMarker = mat->GetEffectiveFXName().c_str();
            packet.perfQueryStart = batch->perfQueryStart;
            packet.perfQueryEnd = batch->perfQueryEnd;

#ifdef __DAVAENGINE_RENDERSTATS__
#ifdef __DAVAENGINE_RENDERSTATS_ALPHABLEND__
            if (packet.userFlags & NMaterial::USER_FLAG_ALPHABLEND)
                packet.queryIndex = VisibilityQueryResults::QUERY_INDEX_ALPHABLEND;
            else if (layerID == RENDER_LAYER_SHADOW_VOLUME_ID)
                packet.queryIndex = VisibilityQueryResults::QUERY_INDEX_LAYER_SHADOW_VOLUME;
            else
                packet.queryIndex = DAVA::InvalidIndex;
#else
            packet.queryIndex = layerID;
#endif
#endif
            rhi::AddPacket(packetList, packet);
        }
    }
}
};
