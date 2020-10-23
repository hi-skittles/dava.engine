#include "rhi_DX9.h"
#include "../Common/rhi_BackendImpl.h"
#include "../Common/rhi_CommonImpl.h"

#include "Debug/DVAssert.h"
#include "Logger/Logger.h"

using DAVA::Logger;

#include "_dx9.h"
#include "../rhi_Type.h"
#include "../Common/rhi_Utils.h"
#include "../Common/RenderLoop.h"
#include "../Common/dbg_StatSet.h"

#include <vector>


#define E_MINSPEC (-3) // Error code for gfx-card that doesn't meet min.spec

namespace rhi
{
//==============================================================================

Dispatch DispatchDX9 = {};

//==============================================================================

struct
DisplayMode
{
    unsigned width;
    unsigned height;
    //    Texture::Format format;
    unsigned refresh_rate;
};

std::vector<DisplayMode> _DisplayMode;

//------------------------------------------------------------------------------

static Api
dx9_HostApi()
{
    return RHI_DX9;
}

//------------------------------------------------------------------------------

static bool
dx9_TextureFormatSupported(TextureFormat format, ProgType progType)
{
    bool supported = false;

    switch (format)
    {
    case TEXTURE_FORMAT_R8G8B8A8:
    case TEXTURE_FORMAT_R5G5B5A1:
    case TEXTURE_FORMAT_R5G6B5:
    case TEXTURE_FORMAT_R4G4B4A4:
    case TEXTURE_FORMAT_R8:
    case TEXTURE_FORMAT_R16:
    case TEXTURE_FORMAT_DXT1:
    case TEXTURE_FORMAT_DXT3:
    case TEXTURE_FORMAT_DXT5:
    case TEXTURE_FORMAT_R32F:
    case TEXTURE_FORMAT_RG32F:
    case TEXTURE_FORMAT_RGBA32F:
        supported = true;
        break;
    default:
        break;
    }

    if (progType == PROG_VERTEX)
    {
        const char* found = strstr(DeviceCaps().deviceDescription, "GeForce");
        if (found && strlen(found) >= strlen("GeForce XXXX")) //filter GeForce 6 and 7 series
        {
            if ((found[8] == '6' || found[8] == '7') && (found[11] == '0' || found[11] == '5'))
            {
                supported = (format == TEXTURE_FORMAT_R32F || format == TEXTURE_FORMAT_RGBA32F);
            }
        }
    }

    return supported;
}

//------------------------------------------------------------------------------

static bool IsValidIntelCardDX9(unsigned vendor_id, unsigned device_id)
{
    return ((vendor_id == 0x8086) && // Intel Architecture

            // These guys are prehistoric :)

            ((device_id == 0x2572) || // 865G
             (device_id == 0x3582) || // 855GM
             (device_id == 0x2562) || // 845G
             (device_id == 0x3577) || // 830M

             // These are from 2005 and later

             (device_id == 0x2A02) || // GM965 Device 0
             (device_id == 0x2A03) || // GM965 Device 1
             (device_id == 0x29A2) || // G965 Device 0
             (device_id == 0x29A3) || // G965 Device 1
             (device_id == 0x27A2) || // 945GM Device 0
             (device_id == 0x27A6) || // 945GM Device 1
             (device_id == 0x2772) || // 945G Device 0
             (device_id == 0x2776) || // 945G Device 1
             (device_id == 0x2592) || // 915GM Device 0
             (device_id == 0x2792) || // 915GM Device 1
             (device_id == 0x2582) || // 915G Device 0
             (device_id == 0x2782) // 915G Device 1
             ));
}

//------------------------------------------------------------------------------

static void dx9_Uninitialize()
{
    QueryBufferDX9::ReleaseQueryPool();
    PerfQueryDX9::ReleasePerfQueryPool();
}

//------------------------------------------------------------------------------

static void dx9_Reset(const ResetParam& param)
{
    bool paramsChanged = false;

    _DX9_ResetParamsMutex.Lock();
    if (_DX9_PresentRectPtr)
    {
        _DX9_PresentRectPtr->right = param.width;
        _DX9_PresentRectPtr->bottom = param.height;

        if (param.width > _DX9_PresentParam.BackBufferWidth || param.height > _DX9_PresentParam.BackBufferHeight)
        {
            _DX9_PresentParam.BackBufferWidth = DAVA::Max(UINT(param.width), _DX9_PresentParam.BackBufferWidth);
            _DX9_PresentParam.BackBufferHeight = DAVA::Max(UINT(param.height), _DX9_PresentParam.BackBufferHeight);

            paramsChanged = true;
        }
    }
    else
    {
        UINT interval = (param.vsyncEnabled) ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
        if (param.width != _DX9_PresentParam.BackBufferWidth
            || param.height != _DX9_PresentParam.BackBufferHeight
            || param.fullScreen != !_DX9_PresentParam.Windowed
            || interval != _DX9_PresentParam.PresentationInterval
            )
        {
            _DX9_PresentParam.BackBufferWidth = param.width;
            _DX9_PresentParam.BackBufferHeight = param.height;
            _DX9_PresentParam.Windowed = !param.fullScreen;
            _DX9_PresentParam.PresentationInterval = interval;

            paramsChanged = true;
        }
    }
    _DX9_ResetParamsMutex.Unlock();

    if (paramsChanged)
        RenderLoop::SetResetPending();
}

//------------------------------------------------------------------------------

static bool dx9_NeedRestoreResources()
{
    uint32 pendingTextures = TextureDX9::NeedRestoreCount();
    uint32 pendingVertexBuffers = VertexBufferDX9::NeedRestoreCount();
    uint32 pendingIndexBuffers = IndexBufferDX9::NeedRestoreCount();

    bool needRestore = (pendingTextures || pendingVertexBuffers || pendingIndexBuffers);
    if (needRestore)
    {
        Logger::Debug("NeedRestore %d TEX, %d VB, %d IB", pendingTextures, pendingVertexBuffers, pendingIndexBuffers);
    }
    return needRestore;
}

static void dx9_SynchronizeCPUGPU(uint64* cpuTimestamp, uint64* gpuTimestamp)
{
    DX9Command cmd = { DX9Command::SYNC_CPU_GPU, { uint64(cpuTimestamp), uint64(gpuTimestamp) } };
    ExecDX9(&cmd, 1, false);
}

//------------------------------------------------------------------------------

void DX9CheckMultisampleSupport(UINT adapter)
{
    const _D3DFORMAT formatsToCheck[] = { D3DFMT_A8R8G8B8, D3DFMT_D24S8 };
    const D3DMULTISAMPLE_TYPE samplesToCheck[] = { D3DMULTISAMPLE_2_SAMPLES, D3DMULTISAMPLE_4_SAMPLES, D3DMULTISAMPLE_8_SAMPLES };

    uint32 sampleCount = 2;

    for (uint32 s = 0; (s < countof(samplesToCheck)) && (sampleCount <= 8); ++s, sampleCount *= 2)
    {
        DWORD qualityLevels = 0;
        for (uint32 f = 0; f < countof(formatsToCheck); ++f)
        {
            HRESULT hr = _D3D9->CheckDeviceMultiSampleType(adapter, D3DDEVTYPE_HAL, formatsToCheck[f], TRUE, samplesToCheck[s], &qualityLevels);
            if (FAILED(hr))
            {
                break;
            }
        }

        if (qualityLevels == 0)
        {
            DAVA::Logger::Info("[RHI-D3D9] Max multisample samples: %u", sampleCount);
            break;
        }
    }

    MutableDeviceCaps::Get().maxSamples = sampleCount / 2;
}

struct AdapterInfo
{
    UINT index;
    D3DCAPS9 caps;
    D3DADAPTER_IDENTIFIER9 info;
};

void dx9_InitCaps(const AdapterInfo& adapterInfo)
{
    DVASSERT(_D3D9_Device);

    Memcpy(MutableDeviceCaps::Get().deviceDescription, adapterInfo.info.Description, DAVA::Min(countof(MutableDeviceCaps::Get().deviceDescription), strlen(adapterInfo.info.Description) + 1));

    D3DCAPS9 caps = {};
    _D3D9_Device->GetDeviceCaps(&caps);

    DWORD shaderModel = DAVA::Min(D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion));
    if (shaderModel < 3)
    {
        if (_DX9_InitParam.renderingErrorCallback)
        {
            _DX9_InitParam.renderingErrorCallback(RenderingError::UnsupportedShaderModel, _DX9_InitParam.renderingErrorCallbackContext);
        }
    }

    MutableDeviceCaps::Get().is32BitIndicesSupported = true;
    MutableDeviceCaps::Get().isFramebufferFetchSupported = true;
    MutableDeviceCaps::Get().isVertexTextureUnitsSupported = (D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion) >= 3);
    MutableDeviceCaps::Get().isInstancingSupported = true;
    MutableDeviceCaps::Get().isUpperLeftRTOrigin = true;
    MutableDeviceCaps::Get().isZeroBaseClipRange = true;
    MutableDeviceCaps::Get().isCenterPixelMapping = true;
    MutableDeviceCaps::Get().maxTextureSize = DAVA::Min(caps.MaxTextureWidth, caps.MaxTextureHeight);

    if (adapterInfo.caps.RasterCaps & D3DPRASTERCAPS_ANISOTROPY)
    {
        MutableDeviceCaps::Get().maxAnisotropy = adapterInfo.caps.MaxAnisotropy;
    }

    {
        IDirect3DQuery9* freqQuery = nullptr;
        _D3D9_Device->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, &freqQuery);
        MutableDeviceCaps::Get().isPerfQuerySupported = (freqQuery != nullptr);
        DAVA::SafeRelease(freqQuery);
    }

    const char* found = strstr(DeviceCaps().deviceDescription, "Radeon");
    if (found && strlen(found) >= strlen("Radeon X1000")) //filter Radeon X1000 Series
    {
        if (found[7] == 'X' && found[8] == '1')
        {
            MutableDeviceCaps::Get().isVertexTextureUnitsSupported = false;
        }
    }

    if (adapterInfo.info.VendorId == 0x8086 && adapterInfo.info.DeviceId == 0x0046) //filter Intel HD Graphics on Pentium Chip
    {
        MutableDeviceCaps::Get().isVertexTextureUnitsSupported = false;
    }

    if (adapterInfo.info.VendorId == 0x10DE && (adapterInfo.info.DeviceId >> 4) == 0x014) //DeviceID from 0x0140 to 0x014F - NV43 chip from NVIDIA
    {
        MutableDeviceCaps::Get().isInstancingSupported = false;
    }

    DX9CheckMultisampleSupport(adapterInfo.index);
}

const char* dx9_AdapterInfo(const D3DADAPTER_IDENTIFIER9& info)
{
    static char buffer[1024] = {};
    memset(buffer, 0, sizeof(buffer));

    sprintf(buffer, "`%s` at `%s` (vendor: 0x%04x, subsystem: 0x%04x, driver: %u.%u.%u.%u)",
            info.Description, info.DeviceName, static_cast<unsigned int>(info.VendorId),
            static_cast<unsigned int>(info.SubSysId),
            HIWORD(info.DriverVersion.HighPart), LOWORD(info.DriverVersion.HighPart),
            HIWORD(info.DriverVersion.LowPart), LOWORD(info.DriverVersion.LowPart));

    return buffer;
}

void dx9_EnumerateAdapters(DAVA::Vector<AdapterInfo>& adapters)
{
    UINT adaptersCount = _D3D9->GetAdapterCount();
    DAVA::Logger::Info("[RHI-D3D9] GetAdapterCount reported %u adapters", adaptersCount);

    for (UINT i = 0; i < adaptersCount; ++i)
    {
        AdapterInfo adapter = { i };
        HRESULT hr = _D3D9->GetAdapterIdentifier(i, 0, &adapter.info);
        if (SUCCEEDED(hr))
        {
            DAVA::Logger::Info("[RHI-D3D9] %s", dx9_AdapterInfo(adapter.info));
            hr = _D3D9->GetDeviceCaps(i, D3DDEVTYPE_HAL, &adapter.caps);
            if (SUCCEEDED(hr))
            {
                adapters.push_back(adapter);
            }
            else
            {
                DAVA::Logger::Error("[RHI-D3D9] GetDeviceCaps with D3DDEVTYPE_HAL failed for %s with error: %s", dx9_AdapterInfo(adapter.info), D3D9ErrorText(hr));

                hr = _D3D9->GetDeviceCaps(i, D3DDEVTYPE_REF, &adapter.caps);
                if (SUCCEEDED(hr))
                {
                    DAVA::Logger::Error("[RHI-D3D9] GetDeviceCaps with D3DDEVTYPE_REF succeeded for %s", dx9_AdapterInfo(adapter.info));
                }
                else
                {
                    DAVA::Logger::Error("[RHI-D3D9] GetDeviceCaps with D3DDEVTYPE_REF failed for %s with error: %s", dx9_AdapterInfo(adapter.info), D3D9ErrorText(hr));
                }
            }
        }
        else
        {
            DAVA::Logger::Error("[RHI-D3D9] GetAdapterIdentifier call failed for adapter %u with error: %s", i, D3D9ErrorText(hr));
        }
    }
}

bool dx9_SelectAdapter(DAVA::Vector<AdapterInfo>& adapters, DWORD& vertex_processing, UINT& indexInVector)
{
    // sort adapters by preferred vendor id
    std::sort(adapters.begin(), adapters.end(), [](const AdapterInfo& l, const AdapterInfo& r) {
        static const UINT preferredVendorIds[] =
        {
          0x10DE, // nVIDIA
          0x1002, // ATI
          0x8086, // Intel
        };
        UINT leftId = UINT(-1);
        UINT rightId = UINT(-1);
        for (UINT i = 0; i < countof(preferredVendorIds); ++i)
        {
            if (preferredVendorIds[i] == l.info.VendorId)
                leftId = i;
            if (preferredVendorIds[i] == r.info.VendorId)
                rightId = i;
        }
        return leftId < rightId;
    });

    // now go through sorted (by preferred vendor) adapters
    // and find one with hadrware vertex processing
    indexInVector = 0;
    vertex_processing = E_FAIL;
    for (const AdapterInfo& adapter : adapters)
    {
        DAVA::Logger::Info("[RHI-D3D9] Attempting to select adapter: %s...", dx9_AdapterInfo(adapter.info));
        if (adapter.caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
        {
            DAVA::Logger::Info("[RHI-D3D9] Selecting adapter %s with D3DCREATE_HARDWARE_VERTEXPROCESSING", dx9_AdapterInfo(adapter.info));
            vertex_processing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
            return true;
        }
        else if (IsValidIntelCardDX9(adapter.info.VendorId, adapter.info.DeviceId))
        {
            DAVA::Logger::Info("[RHI-D3D9] Selecting valid Intel adapter %s with D3DCREATE_SOFTWARE_VERTEXPROCESSING", dx9_AdapterInfo(adapter.info));
            vertex_processing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }
        else
        {
            Logger::Error("[RHI-D3D9] Adapter %s does not meet minimum specs: Intel(R) 845G or Hardware T&L chip required", dx9_AdapterInfo(adapter.info));
        }
        ++indexInVector;
    }
    return (vertex_processing != E_FAIL);
}

void dx9_InitContext()
{
    LoadLibrary(L"D3DCompiler_43.dll");
    _D3D9 = Direct3DCreate9(D3D_SDK_VERSION);

    if (_D3D9 == nullptr)
    {
        Logger::Error("[RHI-D3D9] Failed to create Direct3D object");
        return;
    }

    DAVA::Vector<AdapterInfo> adapters;
    dx9_EnumerateAdapters(adapters);

    UINT index = 0;
    DWORD vertex_processing = E_FAIL;
    if (dx9_SelectAdapter(adapters, vertex_processing, index))
    {
        const AdapterInfo& adapter = adapters.at(index);

        // CRAP: hardcoded params
        HWND wnd = (HWND)_DX9_InitParam.window;

        _DX9_PresentParam.hDeviceWindow = wnd;
        _DX9_PresentParam.Windowed = TRUE;
        _DX9_PresentParam.BackBufferFormat = D3DFMT_UNKNOWN;
        _DX9_PresentParam.BackBufferWidth = _DX9_InitParam.width;
        _DX9_PresentParam.BackBufferHeight = _DX9_InitParam.height;
        _DX9_PresentParam.SwapEffect = D3DSWAPEFFECT_DISCARD;
        _DX9_PresentParam.BackBufferCount = _DX9_InitParam.vsyncEnabled ? 2 : 1;
        _DX9_PresentParam.EnableAutoDepthStencil = TRUE;
        _DX9_PresentParam.AutoDepthStencilFormat = D3DFMT_D24S8;
        _DX9_PresentParam.PresentationInterval = (_DX9_InitParam.vsyncEnabled) ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

        if (_DX9_InitParam.useBackBufferExtraSize)
        {
            DVASSERT(!_DX9_InitParam.fullScreen);

            D3DDISPLAYMODE displayMode;
            _D3D9->GetAdapterDisplayMode(adapter.index, &displayMode);

            _DX9_PresentRect.left = 0;
            _DX9_PresentRect.top = 0;
            _DX9_PresentRect.right = _DX9_InitParam.width;
            _DX9_PresentRect.bottom = _DX9_InitParam.height;
            _DX9_PresentRectPtr = &_DX9_PresentRect;

            _DX9_PresentParam.BackBufferWidth = displayMode.Width;
            _DX9_PresentParam.BackBufferHeight = displayMode.Height;
            _DX9_PresentParam.SwapEffect = D3DSWAPEFFECT_COPY;
            _DX9_PresentParam.BackBufferCount = 1;
            _DX9_PresentParam.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        }

        // TODO: check z-buf formats and create most suitable

        HRESULT hr = _D3D9->CreateDevice(adapter.index, D3DDEVTYPE_HAL, wnd, vertex_processing, &_DX9_PresentParam, &_D3D9_Device);
        if (FAILED(hr))
        {
            //try second time, cause CreateDevice can change present params struct to valid values
            hr = _D3D9->CreateDevice(adapter.index, D3DDEVTYPE_HAL, wnd, vertex_processing, &_DX9_PresentParam, &_D3D9_Device);
        }

        if (SUCCEEDED(hr))
        {
            dx9_InitCaps(adapter);

            DAVA::Logger::Info("[RHI-D3D9] Device created with adapter: %s", dx9_AdapterInfo(adapter.info));
        }
        else
        {
            Logger::Error("[RHI-D3D9] Failed to create device with adapter %s, reported error: %s", dx9_AdapterInfo(adapter.info), D3D9ErrorText(hr));
        }
    }
    else
    {
        uint32 adaptersCount = static_cast<uint32>(adapters.size());
        Logger::Error("[RHI-D3D9] Failed to select adapter, selecting from %u adapters: ", adaptersCount);
        for (const AdapterInfo& adapter : adapters)
            DAVA::Logger::Error("[RHI-D3D9] %s", dx9_AdapterInfo(adapter.info));
    }

    if (_D3D9_Device == nullptr && _DX9_InitParam.renderingErrorCallback)
    {
        _DX9_InitParam.renderingErrorCallback(RenderingError::FailedToCreateDevice, _DX9_InitParam.renderingErrorCallbackContext);
    }
}

bool dx9_CheckSurface()
{
    return true;
}

//------------------------------------------------------------------------------

void dx9_Initialize(const InitParam& param)
{
    _DX9_InitParam = param;

    VertexBufferDX9::SetupDispatch(&DispatchDX9);
    IndexBufferDX9::SetupDispatch(&DispatchDX9);
    QueryBufferDX9::SetupDispatch(&DispatchDX9);
    PerfQueryDX9::SetupDispatch(&DispatchDX9);
    TextureDX9::SetupDispatch(&DispatchDX9);
    PipelineStateDX9::SetupDispatch(&DispatchDX9);
    ConstBufferDX9::SetupDispatch(&DispatchDX9);
    DepthStencilStateDX9::SetupDispatch(&DispatchDX9);
    SamplerStateDX9::SetupDispatch(&DispatchDX9);
    RenderPassDX9::SetupDispatch(&DispatchDX9);
    CommandBufferDX9::SetupDispatch(&DispatchDX9);

    DispatchDX9.impl_Uninitialize = &dx9_Uninitialize;
    DispatchDX9.impl_Reset = &dx9_Reset;
    DispatchDX9.impl_HostApi = &dx9_HostApi;
    DispatchDX9.impl_NeedRestoreResources = &dx9_NeedRestoreResources;
    DispatchDX9.impl_TextureFormatSupported = &dx9_TextureFormatSupported;
    DispatchDX9.impl_SyncCPUGPU = &dx9_SynchronizeCPUGPU;

    DispatchDX9.impl_InitContext = &dx9_InitContext;
    DispatchDX9.impl_ValidateSurface = &dx9_CheckSurface;

    SetDispatchTable(DispatchDX9);

    if (param.maxVertexBufferCount)
        VertexBufferDX9::Init(param.maxVertexBufferCount);
    if (param.maxIndexBufferCount)
        IndexBufferDX9::Init(param.maxIndexBufferCount);
    if (param.maxConstBufferCount)
        ConstBufferDX9::Init(param.maxConstBufferCount);
    if (param.maxTextureCount)
        TextureDX9::Init(param.maxTextureCount);

    if (param.maxSamplerStateCount)
        SamplerStateDX9::Init(param.maxSamplerStateCount);
    if (param.maxPipelineStateCount)
        PipelineStateDX9::Init(param.maxPipelineStateCount);
    if (param.maxDepthStencilStateCount)
        DepthStencilStateDX9::Init(param.maxDepthStencilStateCount);
    if (param.maxRenderPassCount)
        RenderPassDX9::Init(param.maxRenderPassCount);
    if (param.maxCommandBuffer)
        CommandBufferDX9::Init(param.maxCommandBuffer);

    uint32 ringBufferSize = 4 * 1024 * 1024;
    if (param.shaderConstRingBufferSize)
        ringBufferSize = param.shaderConstRingBufferSize;
    ConstBufferDX9::InitializeRingBuffer(ringBufferSize);

    stat_DIP = StatSet::AddStat("rhi'dip", "dip");
    stat_DP = StatSet::AddStat("rhi'dp", "dp");
    stat_SET_PS = StatSet::AddStat("rhi'set-ps", "set-ps");
    stat_SET_TEX = StatSet::AddStat("rhi'set-tex", "set-tex");
    stat_SET_CB = StatSet::AddStat("rhi'set-cb", "set-cb");
}

//==============================================================================
} // namespace rhi
