#pragma once

#include "Base/BaseTypes.h"
#include "Math/Vector.h"
#include "Render/Highlevel/RenderObject.h"

namespace DAVA
{
class NMaterial;
class RenderBatch;
class Camera;
}

namespace OverdrawPerformanceTester
{
class OverdrawTesterRenderObject : public DAVA::RenderObject
{
public:
    OverdrawTesterRenderObject(DAVA::float32 addOverdrawPercent_, DAVA::uint32 maxStepsCount_, DAVA::uint16 textureResolution_);
    ~OverdrawTesterRenderObject();

    void PrepareToRender(DAVA::Camera* camera) override;
    void RecalculateWorldBoundingBox() override;

    void BindDynamicParameters(DAVA::Camera* camera, DAVA::RenderBatch* batch) override;

    DAVA::uint32 GetCurrentStepsCount() const;
    void SetCurrentStepsCount(DAVA::uint32 newCount);

    DAVA::NMaterial* GetDrawMaterial() const;
    void SetDrawMaterial(DAVA::NMaterial* newMat);

    void RecalcBoundingBox() override;

private:
    struct QuadVertex
    {
        DAVA::Vector3 position;
        DAVA::Vector2 texcoord;
    };

    void GenerateQuad(DAVA::uint32 index, DAVA::uint32 layoutId);
    DAVA::Vector<OverdrawTesterRenderObject::QuadVertex> GetQuadVerts(DAVA::uint32 index);
    void GenerateIndexBuffer();
    void Restore();

    DAVA::Vector<QuadVertex> activeVerts;
    DAVA::NMaterial* material = nullptr;
    DAVA::uint32 vertexStride = 0;
    DAVA::uint32 currentStepsCount = 0;

    DAVA::float32 addOverdrawPercentNormalized;
    DAVA::uint16 textureResolution;

    DAVA::Vector<DAVA::RenderBatch*> quads;

    rhi::HIndexBuffer iBuffer;
};

inline DAVA::uint32 OverdrawTesterRenderObject::GetCurrentStepsCount() const
{
    return currentStepsCount;
}

inline void OverdrawTesterRenderObject::SetCurrentStepsCount(DAVA::uint32 newCount)
{
    currentStepsCount = newCount;
}

inline DAVA::NMaterial* OverdrawTesterRenderObject::GetDrawMaterial() const
{
    return material;
}

inline void OverdrawTesterRenderObject::SetDrawMaterial(DAVA::NMaterial* newMat)
{
    material = newMat;
    for (auto batch : quads)
        batch->SetMaterial(material);
}

inline void OverdrawTesterRenderObject::RecalcBoundingBox()
{
}
}
