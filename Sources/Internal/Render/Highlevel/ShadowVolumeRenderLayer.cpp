#include "Render/Highlevel/ShadowVolumeRenderLayer.h"
#include "Render/Highlevel/Camera.h"
#include "Render/Highlevel/RenderBatchArray.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Render/Renderer.h"

namespace DAVA
{
ShadowVolumeRenderLayer::ShadowVolumeRenderLayer(eRenderLayerID id, uint32 sortingFlags)
    : RenderLayer(id, sortingFlags)
{
    PrepareRenderData();
    Renderer::GetSignals().needRestoreResources.Connect(this, &ShadowVolumeRenderLayer::Restore);
}

ShadowVolumeRenderLayer::~ShadowVolumeRenderLayer()
{
    rhi::DeleteVertexBuffer(quadBuffer);
    SafeRelease(shadowRectMaterial);
    Renderer::GetSignals().needRestoreResources.Disconnect(this);
}

const static uint32 VERTEX_COUNT = 6;
std::array<Vector3, VERTEX_COUNT> quad =
{
  Vector3(-1.f, -1.f, 1.f), Vector3(-1.f, 1.f, 1.f), Vector3(1.f, -1.f, 1.f),
  Vector3(-1.f, 1.f, 1.f), Vector3(1.f, 1.f, 1.f), Vector3(1.f, -1.f, 1.f),
};

void ShadowVolumeRenderLayer::Restore()
{
    if ((quadBuffer != rhi::InvalidHandle) && rhi::NeedRestoreVertexBuffer(quadBuffer))
    {
        rhi::UpdateVertexBuffer(quadBuffer, quad.data(), 0, sizeof(Vector3) * VERTEX_COUNT);
    }
}

void ShadowVolumeRenderLayer::PrepareRenderData()
{
    rhi::VertexBuffer::Descriptor vDesc;
    vDesc.size = sizeof(Vector3) * VERTEX_COUNT;
    vDesc.initialData = quad.data();
    vDesc.usage = rhi::USAGE_STATICDRAW;
    quadBuffer = rhi::CreateVertexBuffer(vDesc);

    rhi::VertexLayout vxLayout;
    vxLayout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    shadowRectPacket.vertexLayoutUID = rhi::VertexLayout::UniqueId(vxLayout);

    shadowRectPacket.vertexStreamCount = 1;
    shadowRectPacket.vertexStream[0] = quadBuffer;
    shadowRectPacket.primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
    shadowRectPacket.primitiveCount = 2;

    shadowRectMaterial = new NMaterial();
    shadowRectMaterial->SetFXName(NMaterialName::SHADOWRECT);
    shadowRectMaterial->PreBuildMaterial(PASS_FORWARD);
}

void ShadowVolumeRenderLayer::Draw(Camera* camera, const RenderBatchArray& renderBatchArray, rhi::HPacketList packetList)
{
    if (!QualitySettingsSystem::Instance()->IsOptionEnabled(QualitySettingsSystem::QUALITY_OPTION_STENCIL_SHADOW) ||
        !Renderer::GetOptions()->IsOptionEnabled(RenderOptions::SHADOWVOLUME_DRAW))
    {
        return;
    }

    if (renderBatchArray.GetRenderBatchCount())
    {
        RenderLayer::Draw(camera, renderBatchArray, packetList);

        shadowRectMaterial->BindParams(shadowRectPacket);
        rhi::AddPacket(packetList, shadowRectPacket);
    }
}
};
