#pragma once

#include "ParticleGroup.h"
#include "Render/Highlevel/RenderObject.h"
#include "Render/DynamicBufferAllocator.h"

namespace rhi
{
class VertexLayout;
};

namespace DAVA
{
struct ParticleLayer;
class Camera;

class ParticleRenderObject : public RenderObject
{
    ParticleEffectData* effectData;
    Vector<RenderBatch*> renderBatchCache;

    void AppendParticleGroup(List<ParticleGroup>::iterator begin, List<ParticleGroup>::iterator end, uint32 particlesCount, const Vector3& cameraDirection, Vector3* basisVectors);
    void AppendStripeParticle(List<ParticleGroup>::iterator begin, List<ParticleGroup>::iterator end, Camera* camera, Vector3* basisVectors);
    void AppendRenderBatch(NMaterial* material, uint32 particlesCount, uint32 vertexLayout, const DynamicBufferAllocator::AllocResultVB& vBuffer);
    void AppendRenderBatch(NMaterial* material, uint32 particlesCount, uint32 vertexLayout, const DynamicBufferAllocator::AllocResultVB& vBuffer, const rhi::HIndexBuffer iBuffer, uint32 startIndex);
    void PrepareRenderData(Camera* camera);
    bool CheckIfSimpleParticle(ParticleLayer* layer) const;
    void SetupThreePontGradient(const ParticleGroup& group, NMaterial* material);

    Vector<uint16> indices;
    uint32 sortingOffset;

    uint32 currRenderBatchId;

public:
    ParticleRenderObject(ParticleEffectData* effect);
    ~ParticleRenderObject();

    void PrepareToRender(Camera* camera) override;

    void SetSortingOffset(uint32 offset);

    void BindDynamicParameters(Camera* camera, RenderBatch* batch) override;

    void RecalcBoundingBox() override;
    void RecalculateWorldBoundingBox() override;

private:
    enum eParticlePropsOffsets
    {
        FRAME_BLEND = 0,
        FLOW,
        NOISE,
        FRESNEL_TO_ALPHA_REMAP_PERSPECTIVE_MAPPING
    };

    struct LayoutElement
    {
        rhi::VertexSemantics usage;
        uint32 usageIndex = 0;
        rhi::VertexDataType type;
        uint32 dimension = 0;

        LayoutElement() = default;

        LayoutElement(rhi::VertexSemantics usage_, uint32 usageIndex_, rhi::VertexDataType type_, uint32 dimension_)
            : usage(usage_)
            , usageIndex(usageIndex_)
            , type(type_)
            , dimension(dimension_)
        {
        }
    };
    Map<uint32, LayoutElement> layoutsData;

    uint32 GetVertexStride(ParticleLayer* layer);
    int32 CalculateParticleCount(const ParticleGroup& group);
    uint32 SelectLayout(const ParticleLayer& layer);
    void UpdateStripeVertex(float32*& dataPtr, Vector3& position, Vector3& uv, float32* color, ParticleLayer* layer, Particle* particle, float32 fresToAlpha);
    Vector3 GetStripeNormalizedSpeed(const StripeData& data);

    Map<uint32, uint32> layoutMap;

    float FresnelShlick(float32 nDotVInv, float32 bias, float32 power) const;
    bool CheckGroup(const ParticleGroup& group) const;
    int32 PrepareBasisIndexes(const ParticleGroup& group, int32(&basises)[4]) const;
};

inline void ParticleRenderObject::RecalcBoundingBox()
{
}

inline void ParticleRenderObject::RecalculateWorldBoundingBox()
{
    worldBBox = bbox;
}

inline float ParticleRenderObject::FresnelShlick(float32 nDotVInv, float32 bias, float32 power) const
{
    return bias + (1.0f - bias) * pow(1.0f - nDotVInv, power);
}

inline bool ParticleRenderObject::CheckIfSimpleParticle(ParticleLayer* layer) const
{
    return layer->type != ParticleLayer::eType::TYPE_PARTICLE_STRIPE;
}

inline bool ParticleRenderObject::CheckGroup(const ParticleGroup& group) const
{
    return group.material && group.head && !group.layer->isDisabled && group.layer->sprite;
}
}
