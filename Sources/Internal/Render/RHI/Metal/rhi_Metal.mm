#include "Logger/Logger.h"
#include "../Common/rhi_Private.h"
#include "../Common/rhi_Pool.h"
#include "../Common/dbg_StatSet.h"
#include "../rhi_Public.h"
#include "rhi_Metal.h"

#include "_metal.h"

#import <UIKit/UIKit.h>

#if !(TARGET_IPHONE_SIMULATOR == 1)

namespace rhi
{
id<MTLDevice> _Metal_Device = nil;
id<MTLCommandQueue> _Metal_DefCmdQueue = nil;
id<MTLTexture> _Metal_DefFrameBuf = nil;
id<MTLTexture> _Metal_DefDepthBuf = nil;
id<MTLTexture> _Metal_DefStencilBuf = nil;
id<MTLDepthStencilState> _Metal_DefDepthState = nil;
CAMetalLayer* _Metal_Layer = nil;
DAVA::Semaphore* _Metal_DrawableDispatchSemaphore = nullptr; //used to prevent building command buffers
const uint32 _Metal_DrawableDispatchSemaphoreFrameCount = 4;

//We provide consts-data for metal directly from buffer, so we have to store consts-data for 3 frames.
//Also now metal can work in render-thread and we have to store one more frame data.
static const DAVA::uint32 METAL_CONSTS_RING_BUFFER_CAPACITY_MULTIPLIER = 4;

InitParam _Metal_InitParam;

Dispatch DispatchMetal = { 0 };

//------------------------------------------------------------------------------

static Api metal_HostApi()
{
    return RHI_METAL;
}

//------------------------------------------------------------------------------

static bool metal_TextureFormatSupported(TextureFormat format, ProgType)
{
    bool supported = false;

    switch (format)
    {
    case TEXTURE_FORMAT_R8G8B8A8:
    case TEXTURE_FORMAT_R5G5B5A1:
    case TEXTURE_FORMAT_R5G6B5:
    case TEXTURE_FORMAT_R4G4B4A4:
    case TEXTURE_FORMAT_R8:

    case TEXTURE_FORMAT_PVRTC_4BPP_RGBA:
    case TEXTURE_FORMAT_PVRTC_2BPP_RGBA:

    case TEXTURE_FORMAT_ETC2_R8G8B8:
    case TEXTURE_FORMAT_ETC2_R8G8B8A1:
    case TEXTURE_FORMAT_EAC_R11_UNSIGNED:
    case TEXTURE_FORMAT_EAC_R11_SIGNED:

    case TEXTURE_FORMAT_D24S8:
    case TEXTURE_FORMAT_D16:

    case TEXTURE_FORMAT_R16F:
    case TEXTURE_FORMAT_RG16F:
    case TEXTURE_FORMAT_RGBA16F:

    case TEXTURE_FORMAT_R32F:
    case TEXTURE_FORMAT_RG32F:
    case TEXTURE_FORMAT_RGBA32F:

        supported = true;
        break;

    default:
        break;
    }

    return supported;
}

//------------------------------------------------------------------------------

static void metal_Uninitialize()
{
    if (_Metal_DrawableDispatchSemaphore != nullptr)
        _Metal_DrawableDispatchSemaphore->Post(_Metal_DrawableDispatchSemaphoreFrameCount); //resume render thread if parked there
}

//------------------------------------------------------------------------------

static bool metal_NeedRestoreResources()
{
    static bool lastNeedRestore = false;
    bool needRestore = TextureMetal::NeedRestoreCount();

    if (needRestore)
        DAVA::Logger::Debug("NeedRestore %d TEX", TextureMetal::NeedRestoreCount());

    if (lastNeedRestore && !needRestore)
        DAVA::Logger::Debug("all RHI-resources restored");

    lastNeedRestore = needRestore;

    return needRestore;
}

//------------------------------------------------------------------------------

static void metal_SynchronizeCPUGPU(uint64* cpuTimestamp, uint64* gpuTimestamp)
{
}

//------------------------------------------------------------------------------

bool rhi_MetalIsSupported()
{
    if (!_Metal_Device)
    {
        NSString* currSysVer = [[UIDevice currentDevice] systemVersion];
        if ([currSysVer compare:@"8.0" options:NSNumericSearch] != NSOrderedAscending)
        {
            _Metal_Device = MTLCreateSystemDefaultDevice();
            [_Metal_Device retain];
        }
    }

    return (_Metal_Device) ? true : false;
    //    return [[UIDevice currentDevice].systemVersion floatValue] >= 8.0;
}

void Metal_InitContext()
{
    _Metal_Layer = static_cast<CAMetalLayer*>(_Metal_InitParam.window);
    [_Metal_Layer retain];

    if (!_Metal_Device)
    {
        _Metal_Device = MTLCreateSystemDefaultDevice();
        [_Metal_Device retain];
    }

    _Metal_Layer.device = _Metal_Device;
    _Metal_Layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _Metal_Layer.framebufferOnly = YES;
    _Metal_Layer.drawableSize = CGSizeMake((CGFloat)_Metal_InitParam.width, (CGFloat)_Metal_InitParam.height);

    _Metal_DefCmdQueue = [_Metal_Device newCommandQueue];

    // create frame-buffer

    int w = _Metal_InitParam.width;
    int h = _Metal_InitParam.height;

    MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:w height:h mipmapped:NO];
    MTLTextureDescriptor* stencilDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatStencil8 width:w height:h mipmapped:NO];

    _Metal_DefDepthBuf = [_Metal_Device newTextureWithDescriptor:depthDesc];
    _Metal_DefStencilBuf = [_Metal_Device newTextureWithDescriptor:stencilDesc];

    // create default depth-state

    MTLDepthStencilDescriptor* depth_desc = [MTLDepthStencilDescriptor new];

    depth_desc.depthCompareFunction = MTLCompareFunctionLessEqual;
    depth_desc.depthWriteEnabled = YES;

    _Metal_DefDepthState = [_Metal_Device newDepthStencilStateWithDescriptor:depth_desc];

    NSString* reqSysVer = @"10.0";
    NSString* currSysVer = [[UIDevice currentDevice] systemVersion];
    BOOL iosVersion10 = FALSE;
    if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending)
        iosVersion10 = TRUE;

    if (iosVersion10 && !([_Metal_Device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily2_v1]))
    {
        DAVA::Logger::Warning("A7 ios 10 detected");
        _Metal_DrawableDispatchSemaphore = new DAVA::Semaphore(_Metal_DrawableDispatchSemaphoreFrameCount);
    }

    NSString* minPromotionSysVer = @"10.3";
    if ([currSysVer compare:minPromotionSysVer options:NSNumericSearch] != NSOrderedAscending)
    {
        ::UIScreen* screen = [ ::UIScreen mainScreen];
        int maxFPS = [screen maximumFramesPerSecond];
        MutableDeviceCaps::Get().maxFPS = uint32(maxFPS);
    }

    DAVA::uint32 maxTextureSize = 4096u;
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 90000
    if ([_Metal_Device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily1_v2] || [_Metal_Device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily2_v2])
        maxTextureSize = DAVA::Max(maxTextureSize, 8192u);
    if ([_Metal_Device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1])
        maxTextureSize = DAVA::Max(maxTextureSize, 16384u);
#endif
    
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 100000
    if ([_Metal_Device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily1_v3] || [_Metal_Device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily2_v3])
        maxTextureSize = DAVA::Max(maxTextureSize, 8192u);
    if ([_Metal_Device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v2])
        maxTextureSize = DAVA::Max(maxTextureSize, 16384u);
#endif

    MutableDeviceCaps::Get().maxTextureSize = maxTextureSize;
}
bool Metal_CheckSurface()
{
    return true;
}

//------------------------------------------------------------------------------

void metal_Initialize(const InitParam& param)
{
    _Metal_InitParam = param;
    DAVA::uint32 ringBufferSize = 2 * 1024 * 1024;
    if (param.shaderConstRingBufferSize)
        ringBufferSize = param.shaderConstRingBufferSize;
    ConstBufferMetal::InitializeRingBuffer(ringBufferSize * METAL_CONSTS_RING_BUFFER_CAPACITY_MULTIPLIER);

    stat_DIP = StatSet::AddStat("rhi'dip", "dip");
    stat_DP = StatSet::AddStat("rhi'dp", "dp");
    stat_DTL = StatSet::AddStat("rhi'dtl", "dtl");
    stat_DTS = StatSet::AddStat("rhi'dts", "dts");
    stat_DLL = StatSet::AddStat("rhi'dll", "dll");
    stat_SET_PS = StatSet::AddStat("rhi'set-ps", "set-ps");
    stat_SET_SS = StatSet::AddStat("rhi'set-ss", "set-ss");
    stat_SET_TEX = StatSet::AddStat("rhi'set-tex", "set-tex");
    stat_SET_CB = StatSet::AddStat("rhi'set-cb", "set-cb");
    stat_SET_VB = StatSet::AddStat("rhi'set-vb", "set-vb");
    stat_SET_IB = StatSet::AddStat("rhi'set-ib", "set-ib");

    VertexBufferMetal::SetupDispatch(&DispatchMetal);
    IndexBufferMetal::SetupDispatch(&DispatchMetal);
    QueryBufferMetal::SetupDispatch(&DispatchMetal);
    PerfQueryMetal::SetupDispatch(&DispatchMetal);
    TextureMetal::SetupDispatch(&DispatchMetal);
    PipelineStateMetal::SetupDispatch(&DispatchMetal);
    ConstBufferMetal::SetupDispatch(&DispatchMetal);
    DepthStencilStateMetal::SetupDispatch(&DispatchMetal);
    SamplerStateMetal::SetupDispatch(&DispatchMetal);
    RenderPassMetal::SetupDispatch(&DispatchMetal);
    CommandBufferMetal::SetupDispatch(&DispatchMetal);

    DispatchMetal.impl_Uninitialize = &metal_Uninitialize;
    DispatchMetal.impl_HostApi = &metal_HostApi;
    DispatchMetal.impl_TextureFormatSupported = &metal_TextureFormatSupported;
    DispatchMetal.impl_NeedRestoreResources = &metal_NeedRestoreResources;
    DispatchMetal.impl_NeedRestoreResources = &metal_NeedRestoreResources;

    DispatchMetal.impl_InitContext = &Metal_InitContext;
    DispatchMetal.impl_ValidateSurface = &Metal_CheckSurface;

    DispatchMetal.impl_SyncCPUGPU = &metal_SynchronizeCPUGPU;

    SetDispatchTable(DispatchMetal);

    if (param.maxVertexBufferCount)
        VertexBufferMetal::Init(param.maxVertexBufferCount);
    if (param.maxIndexBufferCount)
        IndexBufferMetal::Init(param.maxIndexBufferCount);
    if (param.maxConstBufferCount)
        ConstBufferMetal::Init(param.maxConstBufferCount);
    if (param.maxTextureCount)
        TextureMetal::Init(param.maxTextureCount);

    if (param.maxSamplerStateCount)
        SamplerStateMetal::Init(param.maxSamplerStateCount);
    if (param.maxPipelineStateCount)
        PipelineStateMetal::Init(param.maxPipelineStateCount);
    if (param.maxDepthStencilStateCount)
        DepthStencilStateMetal::Init(param.maxDepthStencilStateCount);
    if (param.maxRenderPassCount)
        RenderPassMetal::Init(param.maxRenderPassCount);
    if (param.maxCommandBuffer)
        CommandBufferMetal::Init(param.maxCommandBuffer);

    MutableDeviceCaps::Get().is32BitIndicesSupported = true;
    MutableDeviceCaps::Get().isFramebufferFetchSupported = true;
    MutableDeviceCaps::Get().isVertexTextureUnitsSupported = true;
    MutableDeviceCaps::Get().isZeroBaseClipRange = true;
    MutableDeviceCaps::Get().isUpperLeftRTOrigin = true;
    MutableDeviceCaps::Get().isCenterPixelMapping = false;
    MutableDeviceCaps::Get().isInstancingSupported = true;
    MutableDeviceCaps::Get().maxAnisotropy = 16;
    MutableDeviceCaps::Get().maxSamples = 4;
}

} // namespace rhi

#endif //#if !(TARGET_IPHONE_SIMULATOR==1)
