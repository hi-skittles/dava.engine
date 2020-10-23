#include "Render/Highlevel/LandscapeThumbnails.h"
#include "Concurrency/LockGuard.h"
#include "Render/Renderer.h"
#include "Render/ShaderCache.h"
#include "Render/Highlevel/Landscape.h"
#include "Render/Highlevel/RenderPassNames.h"

namespace DAVA
{
namespace LandscapeThumbnails
{
static RequestID RequestIDCounter = InvalidID + 1;

class MaterialFlagsDisabler
{
public:
    MaterialFlagsDisabler(const ScopedPtr<NMaterial>& material_, const Vector<FastName>& flagNames)
        : material(material_)
    {
        flagsInfo.reserve(flagNames.size());
        for (const FastName& flagName : flagNames)
        {
            FlagInfo info;
            info.flagName = flagName;
            info.hasFlag = material->HasLocalFlag(info.flagName);
            if (info.hasFlag)
            {
                info.oldValue = material->GetLocalFlagValue(info.flagName);
                material->SetFlag(info.flagName, 0);
            }
            else
            {
                material->AddFlag(info.flagName, 0);
            }

            flagsInfo.push_back(info);
        }

        material->PreBuildMaterial(PASS_FORWARD);
    }

private:
    ScopedPtr<NMaterial> material;
    struct FlagInfo
    {
        int32 oldValue = 0;
        FastName flagName;
        bool hasFlag = false;
    };

    Vector<FlagInfo> flagsInfo;
};

struct ThumbnailRequest
{
    rhi::HSyncObject syncObject;
    Landscape* landscape = nullptr;
    Texture* texture = nullptr;
    LandscapeThumbnails::Callback callback;
    MaterialFlagsDisabler flagsDisabler;
    RequestID requestID = InvalidID;
    Atomic<bool> cancelled{ false };

    ThumbnailRequest(
    rhi::HSyncObject so,
    Landscape* l,
    const ScopedPtr<NMaterial>& thumbnailMaterial,
    const Vector<FastName>& flagsToDisable,
    Texture* tex,
    LandscapeThumbnails::Callback cb,
    RequestID requestId)
        : syncObject(so)
        , landscape(l)
        , texture(tex)
        , callback(cb)
        , flagsDisabler(thumbnailMaterial, flagsToDisable)
        , requestID(requestId)
    {
    }
};

struct Requests
{
    Mutex mutex;
    List<ThumbnailRequest> list;
};

static Requests requests;

void OnCreateLandscapeTextureCompleted(rhi::HSyncObject syncObject)
{
    List<ThumbnailRequest> completedRequests;
    {
        LockGuard<Mutex> lock(requests.mutex);
        auto i = requests.list.begin();
        while (i != requests.list.end())
        {
            if (i->syncObject == syncObject)
            {
                completedRequests.splice(completedRequests.end(), requests.list, i++);
            }
            else
            {
                ++i;
            }
        }
    }

    for (const auto& req : completedRequests)
    {
        if (req.cancelled.Get() == false)
        {
            req.callback(req.landscape, req.texture);
        }
    }
}

RequestID Create(Landscape* landscape, LandscapeThumbnails::Callback handler)
{
    const uint32 TEXTURE_TILE_FULL_SIZE = 2048;

    ScopedPtr<PolygonGroup> renderData(new PolygonGroup());
    renderData->AllocateData(EVF_VERTEX | EVF_TEXCOORD0, 4, 6);
    renderData->SetPrimitiveType(rhi::PrimitiveType::PRIMITIVE_TRIANGLELIST);
    renderData->SetCoord(0, Vector3(-1.0f, -1.0f, 0.0f));
    renderData->SetCoord(1, Vector3(1.0f, -1.0f, 0.0f));
    renderData->SetCoord(2, Vector3(-1.0f, 1.0f, 0.0f));
    renderData->SetCoord(3, Vector3(1.0f, 1.0f, 0.0f));
    renderData->SetTexcoord(0, 0, Vector2(0.0f, 0.0f));
    renderData->SetTexcoord(0, 1, Vector2(1.0f, 0.0f));
    renderData->SetTexcoord(0, 2, Vector2(0.0f, 1.0f));
    renderData->SetTexcoord(0, 3, Vector2(1.0f, 1.0f));
    renderData->SetIndex(0, 0);
    renderData->SetIndex(1, 1);
    renderData->SetIndex(2, 2);
    renderData->SetIndex(3, 2);
    renderData->SetIndex(4, 1);
    renderData->SetIndex(5, 3);
    renderData->BuildBuffers();

    RequestID requestID = RequestIDCounter++;

    ScopedPtr<NMaterial> thumbnailMaterial(landscape->GetMaterial()->Clone());
    rhi::HSyncObject syncObject = rhi::CreateSyncObject();
    Texture* texture = Texture::CreateFBO(TEXTURE_TILE_FULL_SIZE, TEXTURE_TILE_FULL_SIZE, FORMAT_RGBA8888);
    {
        Vector<FastName> flagsToDisable{ NMaterialFlagName::FLAG_VERTEXFOG,
                                         NMaterialFlagName::FLAG_LANDSCAPE_USE_INSTANCING
        };

        LockGuard<Mutex> lock(requests.mutex);
        requests.list.emplace_back(syncObject, landscape, thumbnailMaterial, flagsToDisable, texture, handler, requestID);
    }
    Renderer::RegisterSyncCallback(syncObject, MakeFunction(&OnCreateLandscapeTextureCompleted));

    const Matrix4* identityMatrix = &Matrix4::IDENTITY;
    Vector3 nullVector(0.0f, 0.0f, 0.0f);
    ShaderDescriptorCache::ClearDynamicBindigs();
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_WORLD, identityMatrix, reinterpret_cast<pointer_size>(identityMatrix));
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_VIEW, identityMatrix, reinterpret_cast<pointer_size>(identityMatrix));
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_PROJ, identityMatrix, reinterpret_cast<pointer_size>(identityMatrix));
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_CAMERA_POS, &nullVector, reinterpret_cast<pointer_size>(&nullVector));

    rhi::Packet packet = {};
    packet.vertexStreamCount = 1;
    packet.vertexStream[0] = renderData->vertexBuffer;
    packet.vertexCount = renderData->vertexCount;
    packet.indexBuffer = renderData->indexBuffer;
    packet.primitiveType = renderData->primitiveType;
    packet.primitiveCount = CalculatePrimitiveCount(renderData->indexCount, renderData->primitiveType);
    packet.vertexLayoutUID = renderData->vertexLayoutId;

    thumbnailMaterial->BindParams(packet);

    rhi::RenderPassConfig passDesc = {};
    passDesc.colorBuffer[0].texture = texture->handle;
    passDesc.priority = PRIORITY_SERVICE_3D;
    passDesc.viewport.width = TEXTURE_TILE_FULL_SIZE;
    passDesc.viewport.height = TEXTURE_TILE_FULL_SIZE;
    passDesc.depthStencilBuffer.texture = rhi::InvalidHandle;

    rhi::HPacketList packetList = {};
    rhi::HRenderPass renderPass = rhi::AllocateRenderPass(passDesc, 1, &packetList);
    rhi::BeginRenderPass(renderPass);
    rhi::BeginPacketList(packetList);
    rhi::AddPacket(packetList, packet);
    rhi::EndPacketList(packetList, syncObject);
    rhi::EndRenderPass(renderPass);

    return requestID;
}

void CancelRequest(RequestID requestID)
{
    LockGuard<Mutex> lock(requests.mutex);
    for (ThumbnailRequest& rq : requests.list)
    {
        if (rq.requestID == requestID)
        {
            rq.cancelled = true;
        }
    }
}
}
}
