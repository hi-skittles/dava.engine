#pragma once

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Render/Highlevel/RenderBatch.h"
#include "Render/Highlevel/RenderBatchArray.h"

namespace DAVA
{
class Camera;
class RenderLayer : public InspBase
{
public:
    enum eRenderLayerID
    {
        RENDER_LAYER_INVALID_ID = -1,
        RENDER_LAYER_OPAQUE_ID = 0,
        RENDER_LAYER_AFTER_OPAQUE_ID = 1,
        RENDER_LAYER_ALPHA_TEST_LAYER_ID = 2,
        RENDER_LAYER_WATER_ID = 3,
        RENDER_LAYER_TRANSLUCENT_ID = 4,
        RENDER_LAYER_AFTER_TRANSLUCENT_ID = 5,
        RENDER_LAYER_SHADOW_VOLUME_ID = 6,
        RENDER_LAYER_VEGETATION_ID = 7,
        RENDER_LAYER_DEBUG_DRAW_ID = 8,
        RENDER_LAYER_ID_COUNT
    };

    static eRenderLayerID GetLayerIDByName(const FastName& name);
    static const FastName& GetLayerNameByID(eRenderLayerID layer);

    // LAYERS SORTING FLAGS
    static const uint32 LAYER_SORTING_FLAGS_OPAQUE;
    static const uint32 LAYER_SORTING_FLAGS_AFTER_OPAQUE;
    static const uint32 LAYER_SORTING_FLAGS_ALPHA_TEST_LAYER;
    static const uint32 LAYER_SORTING_FLAGS_WATER;
    static const uint32 LAYER_SORTING_FLAGS_TRANSLUCENT;
    static const uint32 LAYER_SORTING_FLAGS_AFTER_TRANSLUCENT;
    static const uint32 LAYER_SORTING_FLAGS_SHADOW_VOLUME;
    static const uint32 LAYER_SORTING_FLAGS_VEGETATION;
    static const uint32 LAYER_SORTING_FLAGS_DEBUG_DRAW;

    RenderLayer(eRenderLayerID id, uint32 sortingFlags);
    virtual ~RenderLayer();

    inline eRenderLayerID GetRenderLayerID() const;
    inline uint32 GetSortingFlags() const;

    virtual void Draw(Camera* camera, const RenderBatchArray& batchArray, rhi::HPacketList packetList);

protected:
    eRenderLayerID layerID;
    uint32 sortFlags;
};

inline RenderLayer::eRenderLayerID RenderLayer::GetRenderLayerID() const
{
    return layerID;
}

inline uint32 RenderLayer::GetSortingFlags() const
{
    return sortFlags;
}
}
