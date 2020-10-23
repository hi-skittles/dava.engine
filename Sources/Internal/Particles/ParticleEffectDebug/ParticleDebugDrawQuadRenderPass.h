#pragma once

#include "Scene3D/Systems/ParticleEffectDebugDrawSystem.h"

#include "Base/FastName.h"
#include "Render/RHI/rhi_Public.h"
#include "Render/Highlevel/RenderPass.h"
#include "Math/Vector.h"

namespace DAVA
{
class RenderSystem;
class NMaterial;

class ParticleDebugDrawQuadRenderPass : public RenderPass
{
public:
    struct ParticleDebugQuadRenderPassConfig
    {
        const DAVA::FastName& name;
        DAVA::RenderSystem* renderSystem;
        DAVA::NMaterial* quadMaterial;
        DAVA::NMaterial* quadHeatMaterial;
        const eParticleDebugDrawMode& drawMode;
    };

    ParticleDebugDrawQuadRenderPass(ParticleDebugQuadRenderPassConfig config);
    ~ParticleDebugDrawQuadRenderPass();

    void Draw(DAVA::RenderSystem* renderSystem) override;

    static const DAVA::FastName PASS_DEBUG_DRAW_QUAD;

private:
    struct VertexPT
    {
        Vector3 position;
        Vector2 uv;
    };
    void PrepareRenderData();
    void Restore();

    DAVA::NMaterial* quadMaterial;
    DAVA::NMaterial* quadHeatMaterial;

    const eParticleDebugDrawMode& drawMode;
    rhi::HVertexBuffer quadBuffer;
    rhi::Packet quadPacket;

    static const int vertsCount;
    static const Array<ParticleDebugDrawQuadRenderPass::VertexPT, 6> quad;
};
}