#include "FileSystem/FileSystem.h"
#include "Render/Image/Image.h"
#include "Render/Renderer.h"
#include "Render/ShaderCache.h"
#include "Render/Highlevel/StaticOcclusionRenderPass.h"
#include "Render/Highlevel/RenderBatchArray.h"
#include "Render/Highlevel/StaticOcclusion.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
const uint32 OCCLUSION_RENDER_TARGET_SIZE = 1024;

StaticOcclusionRenderPass::StaticOcclusionRenderPass(const FastName& name)
    : RenderPass(name)
{
    meshRenderBatches.reserve(1024);
    terrainBatches.reserve(256);

    uint32 sortingFlags = RenderBatchArray::SORT_THIS_FRAME | RenderBatchArray::SORT_BY_DISTANCE_FRONT_TO_BACK;
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_OPAQUE_ID, sortingFlags));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_AFTER_OPAQUE_ID, sortingFlags));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_ALPHA_TEST_LAYER_ID, sortingFlags));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_WATER_ID, sortingFlags));

    sortingFlags = RenderBatchArray::SORT_THIS_FRAME | RenderBatchArray::SORT_BY_DISTANCE_BACK_TO_FRONT;
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_TRANSLUCENT_ID, sortingFlags));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_AFTER_TRANSLUCENT_ID, sortingFlags));

    rhi::Texture::Descriptor descriptor;

    descriptor.isRenderTarget = 1;
    descriptor.width = OCCLUSION_RENDER_TARGET_SIZE;
    descriptor.height = OCCLUSION_RENDER_TARGET_SIZE;
    descriptor.autoGenMipmaps = false;
    descriptor.type = rhi::TEXTURE_TYPE_2D;
    descriptor.format = rhi::TEXTURE_FORMAT_R8G8B8A8;
    colorBuffer = rhi::CreateTexture(descriptor);

    descriptor.isRenderTarget = 0;
    descriptor.format = rhi::TEXTURE_FORMAT_D24S8;
    depthBuffer = rhi::CreateTexture(descriptor);

    passConfig.colorBuffer[0].texture = colorBuffer;
    passConfig.colorBuffer[0].loadAction = rhi::LOADACTION_NONE;
    passConfig.colorBuffer[0].storeAction = rhi::STOREACTION_NONE;

    passConfig.depthStencilBuffer.texture = depthBuffer;
    passConfig.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
    passConfig.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;

    passConfig.viewport.width = OCCLUSION_RENDER_TARGET_SIZE;
    passConfig.viewport.height = OCCLUSION_RENDER_TARGET_SIZE;
    passConfig.priority = PRIORITY_SERVICE_3D;

    rhi::DepthStencilState::Descriptor ds;
    ds.depthWriteEnabled = 0;
    stateDisabledDepthWrite = rhi::AcquireDepthStencilState(ds);
}

StaticOcclusionRenderPass::~StaticOcclusionRenderPass()
{
    rhi::DeleteTexture(colorBuffer);
    rhi::DeleteTexture(depthBuffer);
    rhi::ReleaseDepthStencilState(stateDisabledDepthWrite);
}

#if (SAVE_OCCLUSION_IMAGES)

rhi::HTexture sharedColorBuffer = rhi::HTexture(rhi::InvalidHandle);
Map<rhi::HSyncObject, String> renderPassFileNames;

void OnOcclusionRenderPassCompleted(rhi::HSyncObject syncObj)
{
    DVASSERT(renderPassFileNames.count(syncObj) > 0);
    DVASSERT(sharedColorBuffer != rhi::HTexture(rhi::InvalidHandle));

    void* data = rhi::MapTexture(sharedColorBuffer, 0);

    Image* img = Image::CreateFromData(OCCLUSION_RENDER_TARGET_SIZE, OCCLUSION_RENDER_TARGET_SIZE,
                                       PixelFormat::FORMAT_RGBA8888, reinterpret_cast<uint8*>(data));
    img->Save(renderPassFileNames.at(syncObj));
    SafeRelease(img);

    rhi::UnmapTexture(sharedColorBuffer);
    rhi::DeleteSyncObject(syncObj);
}
#endif

bool StaticOcclusionRenderPass::ShouldDisableDepthWrite(RenderBatch* batch)
{
    if (batchesWithoutDepth.count(batch) > 0)
    {
        return true;
    }

    if (processedBatches.count(batch) > 0)
    {
        return false;
    }

    RenderObject* ro = batch->GetRenderObject();
    uint32 rbCount = ro->GetRenderBatchCount();

    bool isSwitchRO = false;
    int32 switchIndex = -1;
    int32 lodIndex = -1;
    Vector<RenderBatch*> roBatches(rbCount);
    for (uint32 i = 0; i < rbCount; ++i)
    {
        roBatches[i] = ro->GetRenderBatch(i, lodIndex, switchIndex);
        if (switchIndex > 0)
            isSwitchRO = true;
    }

    if (isSwitchRO)
        batchesWithoutDepth.insert(roBatches.begin(), roBatches.end());
    else
        processedBatches.insert(roBatches.begin(), roBatches.end());

    return ShouldDisableDepthWrite(batch);
}

void StaticOcclusionRenderPass::DrawOcclusionFrame(RenderSystem* renderSystem, Camera* occlusionCamera,
                                                   StaticOcclusionFrameResult& target, const StaticOcclusionData& data,
                                                   uint32 blockIndex)
{
    terrainBatches.clear();
    meshRenderBatches.clear();

    ShaderDescriptorCache::ClearDynamicBindigs();
    SetupCameraParams(occlusionCamera, occlusionCamera);
    PrepareVisibilityArrays(occlusionCamera, renderSystem);

    UnorderedSet<uint32> invisibleObjects;
    Vector3 cameraPosition = occlusionCamera->GetPosition();

    for (RenderLayer* layer : renderLayers)
    {
        const RenderBatchArray& renderBatchArray = layersBatchArrays[layer->GetRenderLayerID()];

        uint32 batchCount = static_cast<uint32>(renderBatchArray.GetRenderBatchCount());
        for (uint32 batchIndex = 0; batchIndex < batchCount; ++batchIndex)
        {
            RenderBatch* batch = renderBatchArray.Get(batchIndex);
            auto renderObject = batch->GetRenderObject();
            auto objectType = renderObject->GetType();

            if (objectType == RenderObject::TYPE_LANDSCAPE)
            {
                terrainBatches.push_back(batch);
            }
            else if (objectType != RenderObject::TYPE_PARTICLE_EMITTER)
            {
                uint32 batchOptions = 0;
                if (ShouldDisableDepthWrite(batch))
                    batchOptions |= RenderBatchOption::OPTION_DISABLE_DEPTH;

                meshRenderBatches.emplace_back(batch, batchOptions);

                Vector3 position = renderObject->GetWorldBoundingBox().GetCenter();
                batch->layerSortingKey = static_cast<uint32>((position - cameraPosition).SquareLength() * 100.0f);

                auto occlusionId = renderObject->GetStaticOcclusionIndex();
                bool occlusionIndexIsInvalid = occlusionId == INVALID_STATIC_OCCLUSION_INDEX;
                bool isAlreadyVisible = occlusionIndexIsInvalid || data.IsObjectVisibleFromBlock(blockIndex, occlusionId);
                if (!isAlreadyVisible)
                {
                    invisibleObjects.insert(occlusionId);
                }
            }
        }
    }

    if (invisibleObjects.empty())
        return;

    std::sort(meshRenderBatches.begin(), meshRenderBatches.end(),
              [](const BatchWithOptions& a, const BatchWithOptions& b) { return a.first->layerSortingKey < b.first->layerSortingKey; });

    target.blockIndex = blockIndex;
    target.queryBuffer = rhi::CreateQueryBuffer(static_cast<uint32>(meshRenderBatches.size()));
    target.frameRequests.resize(meshRenderBatches.size(), nullptr);

    passConfig.queryBuffer = target.queryBuffer;
    renderPass = rhi::AllocateRenderPass(passConfig, 1, &packetList);
    rhi::BeginRenderPass(renderPass);
    rhi::BeginPacketList(packetList);

    for (auto batch : terrainBatches)
    {
        rhi::Packet packet;
        RenderObject* renderObject = batch->GetRenderObject();
        renderObject->BindDynamicParameters(occlusionCamera, batch);
        NMaterial* mat = batch->GetMaterial();
        DVASSERT(mat);
        batch->BindGeometryData(packet);
        DVASSERT(packet.primitiveCount);
        mat->BindParams(packet);
        packet.cullMode = rhi::CULL_NONE;
        rhi::AddPacket(packetList, packet);
    }

    uint16 k = 0;
    for (const auto& batch : meshRenderBatches)
    {
        RenderObject* renderObject = batch.first->GetRenderObject();
        renderObject->BindDynamicParameters(occlusionCamera, batch.first);

        rhi::Packet packet;
        batch.first->BindGeometryData(packet);
        DVASSERT(packet.primitiveCount);
        batch.first->GetMaterial()->BindParams(packet);
        if (invisibleObjects.count(renderObject->GetStaticOcclusionIndex()) > 0)
        {
            packet.queryIndex = k;
            target.frameRequests[packet.queryIndex] = renderObject;
        }
        packet.cullMode = rhi::CULL_NONE;

        bool isAlphaTestOrAlphaBlend = (packet.userFlags & NMaterial::USER_FLAG_ALPHATEST) != 0;
        isAlphaTestOrAlphaBlend |= (packet.userFlags & NMaterial::USER_FLAG_ALPHABLEND) != 0;

        if ((batch.second & OPTION_DISABLE_DEPTH) == OPTION_DISABLE_DEPTH || isAlphaTestOrAlphaBlend)
            packet.depthStencilState = stateDisabledDepthWrite;

        rhi::AddPacket(packetList, packet);
        ++k;
    }

#if (SAVE_OCCLUSION_IMAGES)
    auto syncObj = rhi::CreateSyncObject();

    auto pos = occlusionCamera->GetPosition();
    auto dir = occlusionCamera->GetDirection();
    auto folder = DAVA::Format("~doc:/occlusion/block-%03d", blockIndex);
    FileSystem::Instance()->CreateDirectory(FilePath(folder), true);
    auto fileName = DAVA::Format("/[%d,%d,%d] from (%d,%d,%d).png",
                                 int32(dir.x), int32(dir.y), int32(dir.z), int32(pos.x), int32(pos.y), int32(pos.z));
    renderPassFileNames.insert({ syncObj, folder + fileName });
    sharedColorBuffer = colorBuffer;

    Renderer::RegisterSyncCallback(syncObj, &OnOcclusionRenderPassCompleted);
    rhi::EndPacketList(packetList, syncObj);
    rhi::EndRenderPass(renderPass);
#else

    rhi::EndPacketList(packetList);
    rhi::EndRenderPass(renderPass);

#endif
}
};
