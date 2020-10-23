#ifndef __DAVAENGINE_SCENE3D_SHADOW_VOLUME_RENDER_PASS_H__
#define __DAVAENGINE_SCENE3D_SHADOW_VOLUME_RENDER_PASS_H__

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Render/Highlevel/RenderPass.h"
#include "Render/UniqueStateSet.h"

namespace DAVA
{
class Camera;
class ShadowVolumeRenderLayer : public RenderLayer
{
public:
    ShadowVolumeRenderLayer(eRenderLayerID id, uint32 sortingFlags);
    virtual ~ShadowVolumeRenderLayer() override;

    void Draw(Camera* camera, const RenderBatchArray& renderBatchArray, rhi::HPacketList packetList) override;

private:
    void PrepareRenderData();
    void Restore();

    NMaterial* shadowRectMaterial = nullptr;
    rhi::Packet shadowRectPacket;
    rhi::HVertexBuffer quadBuffer;
};

} // ns

#endif /* __DAVAENGINE_SCENE3D_SHADOW_VOLUME_RENDER_PASS_H__ */
