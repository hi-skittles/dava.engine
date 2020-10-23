#ifndef __DAVAENGINE_STATIC_OCCLUSION_RENDER_PASS__
#define __DAVAENGINE_STATIC_OCCLUSION_RENDER_PASS__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/BaseMath.h"
#include "Render/Highlevel/RenderPass.h"
#include "Render/RenderBase.h"
#include "Render/Texture.h"

namespace DAVA
{
// use only for debug purposes
// enabling this will save each rendered frame to documents folder
#define SAVE_OCCLUSION_IMAGES 0

struct StaticOcclusionFrameResult;
class StaticOcclusionData;

class StaticOcclusionRenderPass : public RenderPass
{
public:
    StaticOcclusionRenderPass(const FastName& name);
    ~StaticOcclusionRenderPass();

    void DrawOcclusionFrame(RenderSystem* renderSystem, Camera* occlusionCamera,
                            StaticOcclusionFrameResult& target, const StaticOcclusionData&, uint32 blockIndex);

private:
    bool ShouldDisableDepthWrite(RenderBatch*);

private:
    enum RenderBatchOption : uint32
    {
        OPTION_DISABLE_DEPTH = 1 << 0,
    };
    using BatchWithOptions = std::pair<RenderBatch*, uint32>;

    rhi::HTexture colorBuffer;
    rhi::HTexture depthBuffer;
    rhi::HDepthStencilState stateDisabledDepthWrite;

    Vector<RenderBatch*> terrainBatches;
    Vector<BatchWithOptions> meshRenderBatches;

    UnorderedSet<RenderBatch*> batchesWithoutDepth;
    UnorderedSet<RenderBatch*> processedBatches;
};
};

#endif //__DAVAENGINE_STATIC_OCCLUSION_RENDER_PASS__
