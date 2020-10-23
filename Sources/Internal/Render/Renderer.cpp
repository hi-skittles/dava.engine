#include "Renderer.h"
#include "Render/RHI/rhi_ShaderCache.h"
#include "Render/RHI/Common/dbg_StatSet.h"
#include "Render/RHI/Common/rhi_Private.h"
#include "Render/ShaderCache.h"
#include "Render/Material/FXCache.h"
#include "Render/DynamicBufferAllocator.h"
#include "Render/GPUFamilyDescriptor.h"
#include "Render/PixelFormatDescriptor.h"
#include "Render/Image/Image.h"
#include "Render/Texture.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/LockGuard.h"
#include "Platform/DeviceInfo.h"
#include "Debug/ProfilerGPU.h"
#include "Debug/ProfilerOverlay.h"
#include "VisibilityQueryResults.h"

namespace DAVA
{
namespace RendererDetails
{
bool initialized = false;
rhi::Api api;
int32 desiredFPS = 60;

RenderOptions renderOptions;
DynamicBindings dynamicBindings;
RuntimeTextures runtimeTextures;
RenderStats stats;

rhi::ResetParam resetParams;

RenderSignals signals;
Mutex restoreMutex;
Mutex postRestoreMutex;
bool restoreInProgress = false;

struct SyncCallback
{
    rhi::HSyncObject syncObject;
    Token callbackToken;
    Function<void(rhi::HSyncObject)> callback;
};

Vector<SyncCallback> syncCallbacks;

void ProcessSignals()
{
    using namespace RendererDetails;

    if (rhi::NeedRestoreResources())
    {
        restoreInProgress = true;
        LockGuard<Mutex> lock(restoreMutex);
        signals.needRestoreResources.Emit();
    }
    else if (restoreInProgress)
    {
        LockGuard<Mutex> lock(postRestoreMutex);
        signals.restoreResoucesCompleted.Emit();
        restoreInProgress = false;
    }

    for (size_t i = 0, sz = syncCallbacks.size(); i < sz;)
    {
        if (rhi::SyncObjectSignaled(syncCallbacks[i].syncObject))
        {
            syncCallbacks[i].callback(syncCallbacks[i].syncObject);
            RemoveExchangingWithLast(syncCallbacks, i);
            --sz;
        }
        else
        {
            ++i;
        }
    }
}
}

namespace Renderer
{
void Initialize(rhi::Api _api, rhi::InitParam& params)
{
    using namespace RendererDetails;

    DVASSERT(!initialized);

    api = _api;

    rhi::Initialize(api, params);
    rhi::ShaderCache::Initialize();
    ShaderDescriptorCache::Initialize();
    FXCache::Initialize();
    PixelFormatDescriptor::SetHardwareSupportedFormats();

    resetParams.width = params.width;
    resetParams.height = params.height;
    resetParams.vsyncEnabled = params.vsyncEnabled;
    resetParams.window = params.window;
    resetParams.fullScreen = params.fullScreen;

    initialized = true;

    //must be called after setting initialized in true
    Vector<eGPUFamily> gpuLoadingOrder;
    gpuLoadingOrder.push_back(DeviceInfo::GetGPUFamily());
#if defined(__DAVAENGINE_ANDROID__)
    if (gpuLoadingOrder[0] != eGPUFamily::GPU_MALI)
    {
        gpuLoadingOrder.push_back(eGPUFamily::GPU_MALI);
    }
#endif //android

    Texture::SetGPULoadingOrder(gpuLoadingOrder);
    Logger::Info("MAX FPS: %d", rhi::DeviceCaps().maxFPS);
}

void Uninitialize()
{
    DVASSERT(RendererDetails::initialized);

    VisibilityQueryResults::Cleanup();
    FXCache::Uninitialize();
    ShaderDescriptorCache::Uninitialize();
    rhi::ShaderCache::Unitialize();
    rhi::Uninitialize();
    RendererDetails::initialized = false;
}

bool IsInitialized()
{
    return RendererDetails::initialized;
}

void Reset(const rhi::ResetParam& params)
{
    RendererDetails::resetParams = params;

    rhi::Reset(params);
}

rhi::Api GetAPI()
{
    DVASSERT(RendererDetails::initialized);
    return RendererDetails::api;
}

int32 GetDesiredFPS()
{
    return RendererDetails::desiredFPS;
}

void SetDesiredFPS(int32 fps)
{
    RendererDetails::desiredFPS = fps;
}

void SetVSyncEnabled(bool enable)
{
    if (RendererDetails::resetParams.vsyncEnabled != enable)
    {
        RendererDetails::resetParams.vsyncEnabled = enable;
        rhi::Reset(RendererDetails::resetParams);
    }
}

bool IsVSyncEnabled()
{
    return RendererDetails::resetParams.vsyncEnabled;
}

RenderOptions* GetOptions()
{
    DVASSERT(RendererDetails::initialized);
    return &RendererDetails::renderOptions;
}

DynamicBindings& GetDynamicBindings()
{
    return RendererDetails::dynamicBindings;
}

RuntimeTextures& GetRuntimeTextures()
{
    return RendererDetails::runtimeTextures;
}

RenderStats& GetRenderStats()
{
    return RendererDetails::stats;
}

RenderSignals& GetSignals()
{
    return RendererDetails::signals;
}

int32 GetFramebufferWidth()
{
    return static_cast<int32>(RendererDetails::resetParams.width);
}

int32 GetFramebufferHeight()
{
    return static_cast<int32>(RendererDetails::resetParams.height);
}

void BeginFrame()
{
    RendererDetails::ProcessSignals();

    DynamicBufferAllocator::BeginFrame();
}

void EndFrame()
{
    using namespace RendererDetails;

    VisibilityQueryResults::EndFrame();
    DynamicBufferAllocator::EndFrame();

    if (ProfilerOverlay::globalProfilerOverlay)
        ProfilerOverlay::globalProfilerOverlay->OnFrameEnd();

    if (ProfilerGPU::globalProfiler)
        ProfilerGPU::globalProfiler->OnFrameEnd();

    rhi::Present();

    for (uint32 i = 0; i < uint32(VisibilityQueryResults::QUERY_INDEX_COUNT); ++i)
    {
        VisibilityQueryResults::eQueryIndex queryIndex = VisibilityQueryResults::eQueryIndex(i);
        stats.visibilityQueryResults[VisibilityQueryResults::GetQueryIndexName(queryIndex)] = VisibilityQueryResults::GetResult(queryIndex);
    }

    stats.drawIndexedPrimitive = StatSet::StatValue(rhi::stat_DIP);
    stats.drawPrimitive = StatSet::StatValue(rhi::stat_DP);

    stats.pipelineStateSet = StatSet::StatValue(rhi::stat_SET_PS);
    stats.samplerStateSet = StatSet::StatValue(rhi::stat_SET_SS);

    stats.constBufferSet = StatSet::StatValue(rhi::stat_SET_CB);
    stats.textureSet = StatSet::StatValue(rhi::stat_SET_TEX);

    stats.vertexBufferSet = StatSet::StatValue(rhi::stat_SET_VB);
    stats.indexBufferSet = StatSet::StatValue(rhi::stat_SET_IB);

    stats.primitiveTriangleListCount = StatSet::StatValue(rhi::stat_DTL);
    stats.primitiveTriangleStripCount = StatSet::StatValue(rhi::stat_DTS);
    stats.primitiveLineListCount = StatSet::StatValue(rhi::stat_DLL);
}

Token RegisterSyncCallback(rhi::HSyncObject syncObject, Function<void(rhi::HSyncObject)> callback)
{
    Token token = TokenProvider<rhi::HSyncObject>::Generate();
    RendererDetails::syncCallbacks.push_back({ syncObject, token, callback });

    return token;
}

void UnRegisterSyncCallback(Token token)
{
    using namespace RendererDetails;

    DVASSERT(TokenProvider<rhi::HSyncObject>::IsValid(token));
    for (size_t i = 0, sz = syncCallbacks.size(); i < sz; ++i)
    {
        if (syncCallbacks[i].callbackToken == token)
        {
            RemoveExchangingWithLast(syncCallbacks, i);
            break;
        }
    }
}

} //ns Renderer

void RenderStats::Reset()
{
    drawIndexedPrimitive = 0U;
    drawPrimitive = 0U;

    pipelineStateSet = 0U;
    samplerStateSet = 0U;

    constBufferSet = 0U;
    textureSet = 0U;

    vertexBufferSet = 0U;
    indexBufferSet = 0U;

    primitiveTriangleListCount = 0U;
    primitiveTriangleStripCount = 0U;
    primitiveLineListCount = 0U;

    dynamicParamBindCount = 0U;
    materialParamBindCount = 0U;

    batches2d = 0U;
    packets2d = 0U;

    visibleRenderObjects = 0U;
    occludedRenderObjects = 0U;

    visibilityQueryResults.clear();
}

} //ns DAVA
