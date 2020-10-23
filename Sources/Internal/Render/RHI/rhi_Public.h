#ifndef __RHI_PUBLIC_H__
#define __RHI_PUBLIC_H__

#include "rhi_Type.h"

namespace rhi
{
////////////////////////////////////////////////////////////////////////////////
// base operation

struct InitParam
{
    uint32 width = 0;
    uint32 height = 0;
    float32 scaleX = 1.f;
    float32 scaleY = 1.f;
    void* window = nullptr;
    void* defaultFrameBuffer = nullptr;
    bool fullScreen = false;
    bool threadedRenderEnabled = false;
    bool vsyncEnabled = true;
    bool useBackBufferExtraSize = false; //dx9
    uint32 threadedRenderFrameCount = 0;

    uint32 maxIndexBufferCount = 0;
    uint32 maxVertexBufferCount = 0;
    uint32 maxConstBufferCount = 0;
    uint32 maxTextureCount = 0;

    uint32 maxTextureSetCount = 0; //+gl+
    uint32 maxSamplerStateCount = 0; //+gl+
    uint32 maxPipelineStateCount = 0; //+gl+
    uint32 maxDepthStencilStateCount = 0; // +gl+
    uint32 maxRenderPassCount = 0; //+gl+
    uint32 maxCommandBuffer = 0; //+gl
    uint32 maxPacketListCount = 0; //+gl

    uint32 shaderConstRingBufferSize = 0;

    void (*acquireContextFunc)() = nullptr;
    void (*releaseContextFunc)() = nullptr;

    void* renderingErrorCallbackContext = nullptr;
    void (*renderingErrorCallback)(RenderingError, void*) = nullptr;
};

struct ResetParam
{
    uint32 width;
    uint32 height;
    float32 scaleX;
    float32 scaleY;
    void* window;
    uint32 fullScreen : 1;
    uint32 vsyncEnabled : 1;

    ResetParam()
        : width(0)
        , height(0)
        , scaleX(1.f)
        , scaleY(1.f)
        , window(nullptr)
        , fullScreen(false)
        , vsyncEnabled(true)
    {
    }
};

struct RenderDeviceCaps
{
    uint32 maxAnisotropy = 1;
    uint32 maxSamples = 1;
    uint32 maxTextureSize = 2048;
    uint32 maxFPS = 60; // DEPRECATED. Will be removed, use DeviceManager::DisplayInfo::maxFps;

    char deviceDescription[128];

    bool is32BitIndicesSupported = false;
    bool isVertexTextureUnitsSupported = false;
    bool isFramebufferFetchSupported = false;
    bool isUpperLeftRTOrigin = false;
    bool isZeroBaseClipRange = false;
    bool isCenterPixelMapping = false;
    bool isInstancingSupported = false;
    bool isPerfQuerySupported = false;

    RenderDeviceCaps()
    {
        memset(deviceDescription, 0, sizeof(deviceDescription));
    }

    bool isAnisotropicFilteringSupported() const
    {
        return maxAnisotropy > 1;
    }

    bool SupportsAntialiasingType(AntialiasingType type) const
    {
        switch (type)
        {
        case AntialiasingType::MSAA_2X:
            return (maxSamples >= 2);

        case AntialiasingType::MSAA_4X:
            return (maxSamples >= 4);

        default:
            return true;
        }
    }
};

bool ApiIsSupported(Api api);
void Initialize(Api api, const InitParam& param);
void Uninitialize();
void Reset(const ResetParam& param);
bool NeedRestoreResources();

void Present(); // execute all submitted command-buffers & do flip/present

Api HostApi();
bool TextureFormatSupported(TextureFormat format, ProgType progType = PROG_FRAGMENT);
const RenderDeviceCaps& DeviceCaps();

//Suspend/Resume function should be called only from valid state, if you see state assert, it means that you have some error in your application flow.
//SuspendRendering are blocking functions. Control flow will not be returned until render thread finishes it's current tasks and settles in suspended state.
//Most OS allow you to finish whatever you do within AppSuspended call, but all rendering commands afterwards can be treated as errors and can lead to random crashes.
void SuspendRendering();
void SuspendRenderingAfterFrame();
void ResumeRendering();

//notify rendering backend that some explicit code can do some rendering not using rhi and thus leave rendering api in different state
//eg: QT in RE/QuickEd do some opengl calls itself, thus values cached in rhi backend not correspond with actual state of opengl and should be invalidated
void InvalidateCache();
void SynchronizeCPUGPU(uint64* cpuTimestamp, uint64* gpuTimestamp);

////////////////////////////////////////////////////////////////////////////////
// resource-handle

template <ResourceType T>
class ResourceHandle
{
public:
    ResourceHandle()
        : handle(InvalidHandle)
    {
    }
    explicit ResourceHandle(Handle h)
        : handle(h)
    {
    }
    bool IsValid() const
    {
        return handle != InvalidHandle;
    }
    operator Handle() const
    {
        return handle;
    }

private:
    Handle handle;
};

////////////////////////////////////////////////////////////////////////////////
// vertex buffer

typedef ResourceHandle<RESOURCE_VERTEX_BUFFER> HVertexBuffer;

HVertexBuffer CreateVertexBuffer(const VertexBuffer::Descriptor& desc);
void DeleteVertexBuffer(HVertexBuffer vb, bool scheduleDeletion = true);

void* MapVertexBuffer(HVertexBuffer vb, uint32 offset, uint32 size);
void UnmapVertexBuffer(HVertexBuffer vb);

void UpdateVertexBuffer(HVertexBuffer vb, const void* data, uint32 offset, uint32 size);

bool NeedRestoreVertexBuffer(HVertexBuffer vb);

////////////////////////////////////////////////////////////////////////////////
// index buffer

typedef ResourceHandle<RESOURCE_INDEX_BUFFER> HIndexBuffer;

HIndexBuffer CreateIndexBuffer(const IndexBuffer::Descriptor& desc);
void DeleteIndexBuffer(HIndexBuffer ib, bool scheduleDeletion = true);

void* MapIndexBuffer(HIndexBuffer ib, uint32 offset, uint32 size);
void UnmapIndexBuffer(HIndexBuffer ib);

void UpdateIndexBuffer(HIndexBuffer ib, const void* data, uint32 offset, uint32 size);

bool NeedRestoreIndexBuffer(HIndexBuffer vb);

////////////////////////////////////////////////////////////////////////////////
// query

typedef ResourceHandle<RESOURCE_QUERY_BUFFER> HQueryBuffer;

HQueryBuffer CreateQueryBuffer(uint32 maxObjectCount);
void ResetQueryBuffer(HQueryBuffer buf);
void DeleteQueryBuffer(HQueryBuffer buf, bool scheduleDeletion = true);

bool QueryBufferIsReady(HQueryBuffer buf);
bool QueryIsReady(HQueryBuffer buf, uint32 objectIndex);
int QueryValue(HQueryBuffer buf, uint32 objectIndex);

////////////////////////////////////////////////////////////////////////////////
// perf-query

typedef ResourceHandle<RESOURCE_PERFQUERY> HPerfQuery;

HPerfQuery CreatePerfQuery();
void DeletePerfQuery(HPerfQuery handle, bool scheduleDeletion = true);
void ResetPerfQuery(HPerfQuery handle);

bool PerfQueryIsReady(HPerfQuery);
uint64 PerfQueryTimeStamp(HPerfQuery);

void SetFramePerfQueries(HPerfQuery startQuery, HPerfQuery endQuery);

////////////////////////////////////////////////////////////////////////////////
// render-pipeline state & const-buffers

typedef ResourceHandle<RESOURCE_PIPELINE_STATE> HPipelineState;
typedef ResourceHandle<RESOURCE_CONST_BUFFER> HConstBuffer;

HPipelineState AcquireRenderPipelineState(const PipelineState::Descriptor& desc);
void ReleaseRenderPipelineState(HPipelineState rps, bool scheduleDeletion = true);

HConstBuffer CreateVertexConstBuffer(HPipelineState rps, uint32 bufIndex);
void CreateVertexConstBuffers(HPipelineState rps, uint32 maxCount, HConstBuffer* constBuf);

HConstBuffer CreateFragmentConstBuffer(HPipelineState rps, uint32 bufIndex);
void CreateFragmentConstBuffers(HPipelineState rps, uint32 maxCount, HConstBuffer* constBuf);

bool UpdateConstBuffer4fv(HConstBuffer constBuf, uint32 constIndex, const float* data, uint32 constCount);
bool UpdateConstBuffer1fv(HConstBuffer constBuf, uint32 constIndex, uint32 constSubIndex, const float* data, uint32 dataCount);
void DeleteConstBuffer(HConstBuffer constBuf, bool scheduleDeletion = true);

////////////////////////////////////////////////////////////////////////////////
// texture-set

typedef ResourceHandle<RESOURCE_TEXTURE> HTexture;
typedef ResourceHandle<RESOURCE_TEXTURE_SET> HTextureSet;

HTexture CreateTexture(const Texture::Descriptor& desc);
void DeleteTexture(HTexture tex, bool scheduleDeletion = true);

void* MapTexture(HTexture tex, uint32 level = 0);
void UnmapTexture(HTexture tex);

void UpdateTexture(HTexture tex, const void* data, uint32 level, TextureFace face = TEXTURE_FACE_NONE);

bool NeedRestoreTexture(HTexture tex);

struct TextureSetDescriptor
{
    uint32 fragmentTextureCount = 0;
    uint32 vertexTextureCount = 0;
    HTexture fragmentTexture[MAX_FRAGMENT_TEXTURE_SAMPLER_COUNT];
    HTexture vertexTexture[MAX_VERTEX_TEXTURE_SAMPLER_COUNT];
};

HTextureSet AcquireTextureSet(const TextureSetDescriptor& desc);
HTextureSet CopyTextureSet(HTextureSet ts);
void ReleaseTextureSet(HTextureSet ts, bool scheduleDeletion = true);
void ReplaceTextureInAllTextureSets(HTexture oldHandle, HTexture newHandle);

////////////////////////////////////////////////////////////////////////////////
//  depthstencil-state

typedef ResourceHandle<RESOURCE_DEPTHSTENCIL_STATE> HDepthStencilState;

HDepthStencilState AcquireDepthStencilState(const DepthStencilState::Descriptor& desc);
HDepthStencilState CopyDepthStencilState(HDepthStencilState ds);
void ReleaseDepthStencilState(HDepthStencilState ds, bool scheduleDeletion = true);

////////////////////////////////////////////////////////////////////////////////
//  sampler-state

typedef ResourceHandle<RESOURCE_SAMPLER_STATE> HSamplerState;

HSamplerState AcquireSamplerState(const SamplerState::Descriptor& desc);
HSamplerState CopySamplerState(HSamplerState ss);
void ReleaseSamplerState(HSamplerState ss, bool scheduleDeletion = true);

////////////////////////////////////////////////////////////////////////////////
// sync-object

typedef ResourceHandle<RESOURCE_SYNC_OBJECT> HSyncObject;

HSyncObject CreateSyncObject();
void DeleteSyncObject(HSyncObject obj);
bool SyncObjectSignaled(HSyncObject obj);

HSyncObject GetCurrentFrameSyncObject();

////////////////////////////////////////////////////////////////////////////////
// render-pass

typedef ResourceHandle<RESOURCE_RENDER_PASS> HRenderPass;
typedef ResourceHandle<RESOURCE_PACKET_LIST> HPacketList;

HRenderPass AllocateRenderPass(const RenderPassConfig& passDesc, uint32 packetListCount, HPacketList* packetList);
void BeginRenderPass(HRenderPass pass);
void EndRenderPass(HRenderPass pass); // no explicit render-pass 'release' needed
bool NeedInvertProjection(const RenderPassConfig& passDesc);

////////////////////////////////////////////////////////////////////////////////
// rendering

struct Packet
{
    enum
    {
        OPT_OVERRIDE_SCISSOR = 1,
        OPT_WIREFRAME = 2
    };

    uint32 vertexStreamCount;
    HVertexBuffer vertexStream[MAX_VERTEX_STREAM_COUNT];
    uint32 vertexCount;
    uint32 baseVertex;
    uint32 startIndex;
    uint32 vertexLayoutUID;
    HIndexBuffer indexBuffer;
    HPipelineState renderPipelineState;
    HDepthStencilState depthStencilState;
    HSamplerState samplerState;
    CullMode cullMode;
    ScissorRect scissorRect;
    uint32 vertexConstCount;
    HConstBuffer vertexConst[MAX_CONST_BUFFER_COUNT];
    uint32 fragmentConstCount;
    HConstBuffer fragmentConst[MAX_CONST_BUFFER_COUNT];
    HTextureSet textureSet;
    PrimitiveType primitiveType;
    uint32 primitiveCount;
    uint32 instanceCount;
    uint32 baseInstance;
    uint32 queryIndex;
    HPerfQuery perfQueryStart;
    HPerfQuery perfQueryEnd;
    uint32 options;
    uint32 userFlags; //ignored by RHI
    const char* debugMarker;

    Packet()
        : vertexStreamCount(0)
        , vertexCount(0)
        , baseVertex(0)
        , startIndex(0)
        , vertexLayoutUID(VertexLayout::InvalidUID)
        , depthStencilState(InvalidHandle)
        , samplerState(InvalidHandle)
        , cullMode(CULL_CCW)
        , vertexConstCount(0)
        , fragmentConstCount(0)
        , primitiveType(PRIMITIVE_TRIANGLELIST)
        , primitiveCount(0)
        , instanceCount(0)
        , baseInstance(0)
        , queryIndex(DAVA::InvalidIndex)
        , options(0)
        , userFlags(0)
        , debugMarker(nullptr)
    {
    }
};

void BeginPacketList(HPacketList packetList);
void AddPackets(HPacketList packetList, const Packet* packet, uint32 packetCount);
void AddPacket(HPacketList packetList, const Packet& packet);
void EndPacketList(HPacketList packetList, HSyncObject syncObject = HSyncObject(InvalidHandle)); // 'packetList' handle invalid after this, no explicit "release" needed

uint32 NativeColorRGBA(float r, float g, float b, float a = 1.0f);
uint32 NativeColorRGBA(uint32 color); //0xAABBGGRR to api-native;

} // namespace rhi

#endif // __RHI_PUBLIC_H__
