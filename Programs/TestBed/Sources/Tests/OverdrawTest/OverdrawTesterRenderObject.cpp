#include "OverdrawTesterRenderObject.h"

#include "Base/BaseTypes.h"
#include "Engine/Engine.h"
#include "Functional/Function.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/Renderer.h"
#include "Render/Material/NMaterial.h"
#include "Render/Highlevel/RenderBatch.h"
#include "Render/DynamicBufferAllocator.h"
#include "Functional/Function.h"
#include "UI/UIControlSystem.h"

namespace OverdrawPerformanceTester
{
using DAVA::NMaterial;
using DAVA::RenderBatch;
using DAVA::Vector2;
using DAVA::Vector3;
using DAVA::uint8;
using DAVA::uint16;
using DAVA::uint32;
using DAVA::int32;
using DAVA::float32;
using DAVA::DynamicBufferAllocator::AllocResultVB;
using DAVA::DynamicBufferAllocator::AllocResultIB;
using DAVA::Camera;
using DAVA::Size2i;

namespace
{
DAVA::Array<uint16, 6> indices = { 0, 3, 1, 1, 3, 2 };
}

OverdrawTesterRenderObject::OverdrawTesterRenderObject(float32 addOverdrawPercent_, uint32 maxStepsCount_, uint16 textureResolution_)
    : addOverdrawPercentNormalized(addOverdrawPercent_ * 0.01f)
    , textureResolution(textureResolution_)
{
    AddFlag(RenderObject::ALWAYS_CLIPPING_VISIBLE);
    AddFlag(RenderObject::CUSTOM_PREPARE_TO_RENDER);

    rhi::VertexLayout layout;
    layout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    layout.AddElement(rhi::VS_TEXCOORD, 0, rhi::VDT_FLOAT, 2);
    uint32 layoutId = rhi::VertexLayout::UniqueId(layout);

    vertexStride = (3 + 2) * sizeof(float);
    GenerateIndexBuffer();

    for (uint32 i = 0; i < maxStepsCount_; i++)
    {
        GenerateQuad(i, layoutId);
    }

    bbox.AddPoint(Vector3(1.0f, 1.0f, 1.0f));

    DAVA::Renderer::GetSignals().needRestoreResources.Connect(this, &OverdrawTesterRenderObject::Restore);
}

OverdrawTesterRenderObject::~OverdrawTesterRenderObject()
{
    for (DAVA::RenderBatch* batch : quads)
    {
        if (batch->vertexBuffer.IsValid())
            rhi::DeleteVertexBuffer(batch->vertexBuffer);
        DAVA::SafeRelease(batch);
    }
    quads.clear();

    if (iBuffer.IsValid())
        rhi::DeleteIndexBuffer(iBuffer);

    DAVA::Renderer::GetSignals().needRestoreResources.Disconnect(this);
}

void OverdrawTesterRenderObject::PrepareToRender(Camera* camera)
{
    activeRenderBatchArray.clear();
    if (material == nullptr || currentStepsCount == 0)
        return;
    activeVerts.clear();

    for (uint32 i = 0; i < currentStepsCount; i++)
        activeRenderBatchArray.push_back(quads[i]);
}

void OverdrawTesterRenderObject::RecalculateWorldBoundingBox()
{
    worldBBox = bbox;
}

void OverdrawTesterRenderObject::BindDynamicParameters(Camera* camera, DAVA::RenderBatch* batch)
{
    DAVA::Renderer::GetDynamicBindings().SetDynamicParam(DAVA::DynamicBindings::PARAM_WORLD, &DAVA::Matrix4::IDENTITY, reinterpret_cast<DAVA::pointer_size>(&DAVA::Matrix4::IDENTITY));
}

void OverdrawTesterRenderObject::GenerateQuad(uint32 index, uint32 layoutId)
{
    auto quad = GetQuadVerts(index);

    rhi::VertexBuffer::Descriptor desc;
    desc.usage = rhi::USAGE_STATICDRAW;
    desc.size = 4 * sizeof(QuadVertex);
    desc.initialData = quad.data();
    rhi::HVertexBuffer vBuffer = rhi::CreateVertexBuffer(desc);

    RenderBatch* renderBatch = new RenderBatch();
    renderBatch->SetRenderObject(this);
    renderBatch->vertexLayoutId = layoutId;
    renderBatch->vertexBuffer = vBuffer;
    renderBatch->indexBuffer = iBuffer;
    renderBatch->indexCount = 6;
    renderBatch->vertexCount = 4;

    quads.push_back(renderBatch);
}

DAVA::Vector<OverdrawTesterRenderObject::QuadVertex> OverdrawTesterRenderObject::GetQuadVerts(uint32 index)
{
    static const float32 threshold = 0.999f; // This threshold prevent quad from drawing in positions like 0.9999-1.9999 except 0-0.1, and force last quad right edge to be drawn at 1.0.

    float32 xStart = addOverdrawPercentNormalized * index;
    xStart = xStart - static_cast<int32>(xStart);
    xStart = xStart < threshold ? xStart : 0.0f;

    xStart = xStart * 2 - 1.0f;
    float32 xEnd = xStart + 2.0f * addOverdrawPercentNormalized;
    xEnd = xEnd < threshold ? xEnd : 1.0f;

    // Try to keep 2pix - 1tex ratio.
    Size2i size = DAVA::GetEngineContext()->uiControlSystem->vcs->GetPhysicalScreenSize();

    float32 maxX = size.dx * 0.5f / textureResolution;
    float32 maxY = size.dy * 0.5f / textureResolution;

    float32 uvStart = (xStart * 0.5f + 0.5f) * maxX;
    float32 uvEnd = (xEnd * 0.5f + 0.5f) * maxX;
    return
    { {
    { { xStart, -1.0f, 1.0f }, { uvStart, 0.0f } },
    { { xStart, 1.0f, 1.0f }, { uvStart, maxY } },
    { { xEnd, 1.0f, 1.0f }, { uvEnd, maxY } },
    { { xEnd, -1.0f, 1.0f }, { uvEnd, 0.0f } }
    } };
}

void OverdrawTesterRenderObject::GenerateIndexBuffer()
{
    rhi::IndexBuffer::Descriptor iDesc;
    iDesc.indexSize = rhi::INDEX_SIZE_16BIT;
    iDesc.size = 6 * sizeof(uint16);
    iDesc.usage = rhi::USAGE_STATICDRAW;
    iDesc.initialData = indices.data();
    iBuffer = rhi::CreateIndexBuffer(iDesc);
}

void OverdrawTesterRenderObject::Restore()
{
    if (rhi::NeedRestoreIndexBuffer(iBuffer))
        rhi::UpdateIndexBuffer(iBuffer, indices.data(), 0, static_cast<DAVA::uint32>(indices.size() * sizeof(uint16)));
    for (size_t i = 0; i < quads.size(); i++)
    {
        if (rhi::NeedRestoreVertexBuffer(quads[i]->vertexBuffer))
            rhi::UpdateVertexBuffer(quads[i]->vertexBuffer, GetQuadVerts(static_cast<DAVA::uint32>(i)).data(), 0, static_cast<DAVA::uint32>(4 * sizeof(QuadVertex)));
    }
}
}