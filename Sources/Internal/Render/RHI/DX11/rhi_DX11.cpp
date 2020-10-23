#include "rhi_DX11.h"
#include "../Common/RenderLoop.h"
#include "../Common/FrameLoop.h"
#include "../Common/dbg_StatSet.h"

#include "Engine/Engine.h"
#include "DeviceManager/DeviceManager.h"
#include "Platform/DeviceInfo.h"

#if defined(__DAVAENGINE_WIN_UAP__)
#include <wrl/client.h>
#include <Windows.ui.xaml.media.dxinterop.h>
#endif

#define RHI_DX11_FORCE_FEATURE_LEVEL_9 0
#define RHI_DX11_ASSERT_ON_ERROR 1

extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace DAVA
{
namespace UWPWorkaround
{
bool enableSurfaceSizeWorkaround = false; // 'workaround' for ATI HD ****G drivers
}
}

namespace rhi
{
using namespace Microsoft::WRL;

static Dispatch DispatchDX11 = {};
static ResetParam resetParams;
static DAVA::Mutex resetParamsSync;
static DWORD _DX11_RenderThreadId = 0;
static D3D_FEATURE_LEVEL _DX11_SupportedFeatureLevels[] =
{
#if (!RHI_DX11_FORCE_FEATURE_LEVEL_9)
  D3D_FEATURE_LEVEL_11_1,
  D3D_FEATURE_LEVEL_11_0,
  D3D_FEATURE_LEVEL_10_1,
  D3D_FEATURE_LEVEL_10_0,
#endif
  D3D_FEATURE_LEVEL_9_3,
  D3D_FEATURE_LEVEL_9_2,
  D3D_FEATURE_LEVEL_9_1
};

DX11Resources dx11;

/*
 * Helper functions
 */
void dx11_InitCaps()
{
    MutableDeviceCaps::Get().is32BitIndicesSupported = true;
    MutableDeviceCaps::Get().isFramebufferFetchSupported = true;
    MutableDeviceCaps::Get().isVertexTextureUnitsSupported = (dx11.usedFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
    MutableDeviceCaps::Get().isUpperLeftRTOrigin = true;
    MutableDeviceCaps::Get().isZeroBaseClipRange = true;
    MutableDeviceCaps::Get().isCenterPixelMapping = false;
    MutableDeviceCaps::Get().isInstancingSupported = (dx11.usedFeatureLevel >= D3D_FEATURE_LEVEL_9_2);
    MutableDeviceCaps::Get().isPerfQuerySupported = (dx11.usedFeatureLevel >= D3D_FEATURE_LEVEL_9_2);
    MutableDeviceCaps::Get().maxAnisotropy = D3D11_REQ_MAXANISOTROPY;

    switch (dx11.usedFeatureLevel)
    {
    case D3D_FEATURE_LEVEL_9_1:
    case D3D_FEATURE_LEVEL_9_2:
        MutableDeviceCaps::Get().maxTextureSize = D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        break;
    case D3D_FEATURE_LEVEL_9_3:
        MutableDeviceCaps::Get().maxTextureSize = D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        break;
    case D3D_FEATURE_LEVEL_10_0:
    case D3D_FEATURE_LEVEL_10_1:
        MutableDeviceCaps::Get().maxTextureSize = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        break;
    case D3D_FEATURE_LEVEL_11_0:
    case D3D_FEATURE_LEVEL_11_1:
        MutableDeviceCaps::Get().maxTextureSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        break;
    }

#if defined(__DAVAENGINE_WIN_UAP__)
    if (DAVA::DeviceInfo::GetPlatform() == DAVA::DeviceInfo::ePlatform::PLATFORM_PHONE_WIN_UAP)
    {
        // explicitly disable multisampling support on win phones
        MutableDeviceCaps::Get().maxSamples = 1;
    }
    else
    #endif
    {
        MutableDeviceCaps::Get().maxSamples = DX11_GetMaxSupportedMultisampleCount(dx11.device.Get());
    }

    //Some drivers returns untrue DX feature-level, so check it manually
    if (MutableDeviceCaps::Get().isPerfQuerySupported)
    {
        ID3D11Query* freqQuery = nullptr;
        D3D11_QUERY_DESC desc = { D3D11_QUERY_TIMESTAMP_DISJOINT };
        dx11.device->CreateQuery(&desc, &freqQuery);
        MutableDeviceCaps::Get().isPerfQuerySupported = (freqQuery != nullptr);
        DAVA::SafeRelease(freqQuery);
    }

    D3D11_FEATURE_DATA_THREADING threadingData = {};
    dx11.device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threadingData, sizeof(threadingData));
    dx11.useHardwareCommandBuffers = threadingData.DriverCommandLists && threadingData.DriverConcurrentCreates;
    DAVA::Logger::Info("[RHI-DX11] hardware command buffers enabled: %d", static_cast<int32>(dx11.useHardwareCommandBuffers));

    if (dx11.useHardwareCommandBuffers)
        CommandBufferDX11::BindHardwareCommandBufferDispatch(&DispatchDX11);
    else
        CommandBufferDX11::BindSoftwareCommandBufferDispatch(&DispatchDX11);

    SetDispatchTable(DispatchDX11);
}

void ExecDX11(DX11Command* command, uint32 cmdCount, bool forceExecute)
{
    CommonImpl::ImmediateCommand cmd;
    cmd.cmdData = command;
    cmd.cmdCount = cmdCount;
    cmd.forceExecute = forceExecute;
    RenderLoop::IssueImmediateCommand(&cmd);
}

bool ExecDX11DeviceCommand(DX11Command cmd, const char* cmdName, const char* fileName, uint32 line)
{
    DVASSERT(cmd.func >= DX11Command::DEVICE_FIRST_COMMAND);
    DVASSERT(cmd.func < DX11Command::DEVICE_LAST_COMMAND);

    if (dx11.useHardwareCommandBuffers || (GetCurrentThreadId() == _DX11_RenderThreadId))
    {
        // running on render thread, or using deferred context
        // immediate execution, device validation will occur inside
        ExecDX11(&cmd, 1, true);
    }
    else
    {
        // call occured from secondary (non-render thread)
        // validate device before sending commands to execution
        ValidateDX11Device(cmdName);
        ExecDX11(&cmd, 1, false);
    }

    DX11_ProcessCallResult(cmd.retval, cmdName, fileName, line);
    return SUCCEEDED(cmd.retval);
}

void ValidateDX11Device(const char* call)
{
    if (dx11.device == nullptr)
    {
        DAVA::Logger::Error("DX11 Device is not ready, %s and further calls will be blocked.", call);
        for (;;)
        {
            Sleep(1);
        }
    }
}

uint32 DX11_GetMaxSupportedMultisampleCount(ID3D11Device* device)
{
    const DXGI_FORMAT formatsToCheck[] = { dx11.BackBufferFormat, dx11.DepthStencilFormat };

    uint32 sampleCount = 2;

    for (uint32 s = 0; (sampleCount <= 8); ++s, sampleCount *= 2)
    {
        UINT numQualityLevels = 0;
        for (uint32 f = 0; f < countof(formatsToCheck); ++f)
        {
            UINT formatSupport = 0;
            HRESULT hr = device->CheckFormatSupport(formatsToCheck[f], &formatSupport);
            if (formatSupport & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
            {
                hr = device->CheckMultisampleQualityLevels(formatsToCheck[f], sampleCount, &numQualityLevels);
                if (FAILED(hr) || (numQualityLevels == 0))
                {
                    break;
                }
            }
        }
        if (numQualityLevels == 0)
        {
            DAVA::Logger::Info("DX11 max multisample samples: %u", sampleCount / 2);
            break;
        }
    }

    return sampleCount / 2;
}

bool DX11_CheckResult(HRESULT hr, const char* call, const char* fileName, const uint32 line)
{
    if (FAILED(hr))
    {
    #if (RHI_DX11_ASSERT_ON_ERROR)
        DAVA::String error = DAVA::Format("D3D11Error at %s: %d\n%s\nCondition: %s", fileName, line, DX11_GetErrorText(hr), call);
        DVASSERT(0, error.c_str());
    #else
        DAVA::Logger::Error("D3D11Error at %s: %d\n%s\nCondition: %s", fileName, line, DX11_GetErrorText(hr), call);
    #endif
        return false;
    }

    return true;
}

void DX11_ProcessCallResult(HRESULT hr, const char* call, const char* fileName, const uint32 line)
{
    if ((hr == DXGI_ERROR_DEVICE_REMOVED) || (hr == DXGI_ERROR_DEVICE_RESET))
    {
        const char* actualError = DX11_GetErrorText(hr);
        const char* reason = DX11_GetErrorText(dx11.device->GetDeviceRemovedReason());

        DAVA::String info = DAVA::Format("DX11 Device removed/reset\n%s\nat %s [%u]:\n\n%s\n\n%s", call, fileName, line, actualError, reason);
        DAVA::Logger::Error(info.c_str());
        DVASSERT(0, info.c_str());

        ReportError(dx11.initParameters, RenderingError::DriverError);
        dx11.device = nullptr;
    }
    else if (FAILED(hr))
    {
        const char* errorText = DX11_GetErrorText(hr);
        DAVA::Logger::Error("DX11 Device call %s\nat %s [%u] failed:\n%s", call, fileName, line, errorText);
    }
}

bool dx11_HasDebugLayers()
{
    return SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_NULL, 0, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, nullptr, nullptr, nullptr));
}

HRESULT CreateDXGIFactoryWrapper(UINT flags)
{
#if defined(__DAVAENGINE_WIN_UAP__)
    return CreateDXGIFactory2(flags, IID_PPV_ARGS(dx11.factory.GetAddressOf()));
#else
    return CreateDXGIFactory1(IID_PPV_ARGS(dx11.factory.GetAddressOf()));
#endif
}

D3D_FEATURE_LEVEL dx11_maxSupportedFeatureLevel(ComPtr<IDXGIAdapter> adapter)
{
    D3D_FEATURE_LEVEL result = D3D_FEATURE_LEVEL_9_1;
    D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, _DX11_SupportedFeatureLevels,
                      countof(_DX11_SupportedFeatureLevels), D3D11_SDK_VERSION, nullptr, &result, nullptr);
    return result;
}

ComPtr<IDXGIAdapter> dx11_SelectAdapter()
{
    struct AdapterWithDesc
    {
        ComPtr<IDXGIAdapter> adapter;
        DXGI_ADAPTER_DESC desc;
        D3D_FEATURE_LEVEL maxFeatureLevel;
    };
    DAVA::Vector<AdapterWithDesc> availableAdapters;

    UINT factoryFlags = 0;
#if defined(__DAVAENGINE_WIN_UAP__) && defined(__DAVAENGINE_DEBUG__)
    if (dx11.hasDebugLayers)
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    if (!DX11Check(CreateDXGIFactoryWrapper(factoryFlags)))
        return ComPtr<IDXGIAdapter>();

    DAVA::Logger::Info("[RHI-DX11] available adapters:");
    UINT index = 0;
    ComPtr<IDXGIAdapter> adapter;
    while (dx11.factory->EnumAdapters(index, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND)
    {
        availableAdapters.emplace_back();
        AdapterWithDesc& entry = availableAdapters.back();
        entry.adapter = adapter;
        entry.maxFeatureLevel = dx11_maxSupportedFeatureLevel(adapter);
        adapter->GetDesc(&entry.desc);

        uint32 featureLevel = static_cast<uint32>(entry.maxFeatureLevel);
        uint32 featureLevelMajor = ((featureLevel >> 8) & 0xf0) >> 4;
        uint32 featureLevelMinor = (featureLevel >> 8) & 0x0f;
        DAVA::Logger::Info("%d : '%S' (vendor: 0x%04X, subsystem: 0x%04X), feature level: 0x%04x (%u.%u)", index,
                           entry.desc.Description, entry.desc.VendorId, entry.desc.SubSysId,
                           featureLevel, featureLevelMajor, featureLevelMinor);
        ++index;
    }

    if (availableAdapters.empty())
        return ComPtr<IDXGIAdapter>();

    std::sort(availableAdapters.begin(), availableAdapters.end(), [](const AdapterWithDesc& l, const AdapterWithDesc& r)
              {
                  static const UINT preferredVendorIds[] =
                  {
                    0x10DE, // Nvidia
                    0x1002, // ATI
                    0x8086, // Intel
                  };
                  UINT leftId = UINT(-1);
                  UINT rightId = UINT(-1);
                  for (UINT i = 0; i < countof(preferredVendorIds); ++i)
                  {
                      if (preferredVendorIds[i] == l.desc.VendorId)
                          leftId = i;
                      if (preferredVendorIds[i] == r.desc.VendorId)
                          rightId = i;
                  }
                  return leftId < rightId;
              });

    const AdapterWithDesc& selected = availableAdapters.front();
    DAVA::Logger::Info("[RHI-DX11] Using adapter `%S` (vendor: %04X, subsystem: %04X)", selected.desc.Description, selected.desc.VendorId, selected.desc.SubSysId);
    return selected.adapter;
}

bool dx11_CreateDeviceWithAdapter(const InitParam& param, ComPtr<IDXGIAdapter> adapter)
{
    UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(__DAVAENGINE_DEBUG__)
    if (dx11.hasDebugLayers)
        deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    if (adapter.Get() == nullptr)
        DAVA::Logger::Info("[RHI-DX11] Creating device without selected adapter");

    D3D_DRIVER_TYPE driverType = adapter.Get() ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;
    HRESULT result = D3D11CreateDevice(adapter.Get(), driverType, nullptr, deviceFlags, _DX11_SupportedFeatureLevels, countof(_DX11_SupportedFeatureLevels),
                                       D3D11_SDK_VERSION, dx11.device.GetAddressOf(), &dx11.usedFeatureLevel, dx11.context.GetAddressOf());

    bool failedToCreateDevice = (dx11.device.Get() == nullptr) || !DX11Check(result);
    if (failedToCreateDevice)
    {
        driverType = D3D_DRIVER_TYPE_HARDWARE;
        result = D3D11CreateDevice(nullptr, driverType, nullptr, deviceFlags, _DX11_SupportedFeatureLevels, countof(_DX11_SupportedFeatureLevels),
                                   D3D11_SDK_VERSION, dx11.device.GetAddressOf(), &dx11.usedFeatureLevel, dx11.context.GetAddressOf());
    }

    failedToCreateDevice = (dx11.device.Get() == nullptr) || !DX11Check(result);
    if (failedToCreateDevice)
    {
        DAVA::Logger::Error("[RHI-DX11] Failed create ID3D11Device");
        ReportError(param, RenderingError::FailedToInitialize);
        return false;
    }

    if (!DX11Check(dx11.device.As(&dx11.dxgiDevice)))
    {
        DAVA::Logger::Error("[RHI-DX11] Failed to retrieve IDXGIDevice1 object from ID3D11Device");
        ReportError(param, RenderingError::FailedToInitialize);
        return false;
    }
    uint32 featureLevel = static_cast<uint32>(dx11.usedFeatureLevel);
    uint32 featureLevelMajor = ((featureLevel >> 8) & 0xf0) >> 4;
    uint32 featureLevelMinor = (featureLevel >> 8) & 0x0f;
    DAVA::Logger::Info("[RHI-DX11] Init with feature level: 0x%04x (%u.%u)", featureLevel, featureLevelMajor, featureLevelMinor);

    // device was created, but no adapter provided
    if (adapter.Get() == nullptr)
    {
        DX11Check(dx11.dxgiDevice->GetAdapter(adapter.GetAddressOf()));
        if (adapter.Get() == nullptr)
        {
            DAVA::Logger::Error("[RHI-DX11] Failed to retrieve IDXGIAdapter object from IDXGIDevice1");
            ReportError(param, RenderingError::FailedToInitialize);
            return false;
        }

        DXGI_ADAPTER_DESC desc = {};
        adapter->GetDesc(&desc);
        DAVA::Logger::Info("[RHI-DX11] Using retrieved adapter `%S` (vendor: 0x%04X, subsystem: 0x%04X)", desc.Description, desc.VendorId, desc.SubSysId);

        if (dx11.factory.Get() == nullptr)
        {
            DAVA::Logger::Info("[RHI-DX11] IDXGIFactory2 was not created, retrieving it from the adapter");
            DX11Check(adapter->GetParent(IID_PPV_ARGS(dx11.factory.GetAddressOf())));
            if (dx11.factory.Get() == nullptr)
            {
                DAVA::Logger::Info("[RHI-DX11] Failed to retrieve IDXGIFactory2 from IDXGIAdapter");
                ReportError(param, RenderingError::FailedToInitialize);
                return false;
            }
        }
    }

    return true;
}

bool dx11_CreateDevice(const InitParam& param)
{
    DAVA::Logger::Info("[RHI-DX11] Creating device...");
    ComPtr<IDXGIAdapter> adapter = dx11_SelectAdapter();
    return dx11_CreateDeviceWithAdapter(param, adapter);
}

void dx11_DestroyDevice()
{
    dx11.renderTarget.Reset();
    dx11.renderTargetView.Reset();
    dx11.depthStencil.Reset();
    dx11.depthStencilView.Reset();
    dx11.factory.Reset();
    dx11.swapChain.Reset();
    dx11.deferredContext.Reset();
    dx11.context.Reset();
    dx11.device.Reset();
}

void dx11_SetSwapChain(const InitParam& param)
{
#if defined(__DAVAENGINE_WIN_UAP__)
    using ::Windows::UI::Core::CoreDispatcherPriority;
    using ::Windows::UI::Core::DispatchedHandler;
    using ::Windows::UI::Xaml::Controls::SwapChainPanel;

    SwapChainPanel ^ swapChainPanel = reinterpret_cast<SwapChainPanel ^>(param.window);
    auto handler = [swapChainPanel, param]() // Capture param by value as calling function may create it on stack
    {
        ComPtr<ISwapChainPanelNative> panelNative;
        if (DX11Check(reinterpret_cast<IUnknown*>(swapChainPanel)->QueryInterface(IID_PPV_ARGS(panelNative.GetAddressOf()))))
            panelNative->SetSwapChain(dx11.swapChain.Get());
        else
            ReportError(param, RenderingError::FailedToInitialize);
    };
    swapChainPanel->Dispatcher->RunAsync(CoreDispatcherPriority::High, ref new DispatchedHandler(handler, Platform::CallbackContext::Any));
#endif
}

bool dx11_CreateSwapChain(const InitParam& param)
{
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = param.width;
    desc.Height = param.height + (DAVA::UWPWorkaround::enableSurfaceSizeWorkaround ? 1 : 0);
    desc.Format = dx11.BackBufferFormat;
    desc.SampleDesc.Count = 1;
    desc.BufferCount = dx11.BackBuffersCount;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    HRESULT result = E_FAIL;
    try
    {
    #if defined(__DAVAENGINE_WIN_UAP__)
        ComPtr<IDXGISwapChain1> swapChain;
        result = dx11.factory->CreateSwapChainForComposition(dx11.device.Get(), &desc, nullptr, swapChain.GetAddressOf());
        if (!DX11Check(swapChain.As(&dx11.swapChain)))
        {
            ReportError(param, RenderingError::FailedToInitialize);
            return false;
        }
    #else
        HWND wnd = reinterpret_cast<HWND>(param.window);
        result = dx11.factory->CreateSwapChainForHwnd(dx11.device.Get(), wnd, &desc, nullptr, nullptr, dx11.swapChain.GetAddressOf());
    #endif
    }
    catch (...)
    {
        dx11_DestroyDevice();
        return false;
    }

    if (!DX11Check(result))
        return false;

    dx11.dxgiDevice->SetMaximumFrameLatency(1);
    return true;
}

bool dx11_ResizeSwapChain(uint32 width, uint32 height, float scaleX, float scaleY)
{
    ID3D11RenderTargetView* nullView[] = { nullptr };
    dx11.context->OMSetRenderTargets(1, nullView, nullptr);
    dx11.renderTarget.Reset();
    dx11.renderTargetView.Reset();
    dx11.depthStencil.Reset();
    dx11.depthStencilView.Reset();
    dx11.context->ClearState();
    dx11.context->Flush();

    height += (DAVA::UWPWorkaround::enableSurfaceSizeWorkaround ? 1 : 0);

    if (!DX11Check(dx11.swapChain->ResizeBuffers(dx11.BackBuffersCount, width, height, dx11.BackBufferFormat, 0)))
        return false;

#if defined(__DAVAENGINE_WIN_UAP__)
    DXGI_MATRIX_3X2_F inverseScale = {};
    inverseScale._11 = 1.0f / scaleX;
    inverseScale._22 = 1.0f / scaleY;
    DX11Check(dx11.swapChain->SetMatrixTransform(&inverseScale));
#endif

    if (!DX11Check(dx11.swapChain->GetBuffer(0, IID_PPV_ARGS(dx11.renderTarget.GetAddressOf()))))
        return false;

    if (!DX11DeviceCommand(DX11Command::CREATE_RENDER_TARGET_VIEW, dx11.renderTarget.Get(), 0, dx11.renderTargetView.GetAddressOf()))
        return false;

    D3D11_TEXTURE2D_DESC dsDesc = {};
    dsDesc.Width = width;
    dsDesc.Height = height;
    dsDesc.Format = dx11.DepthStencilFormat;
    dsDesc.MipLevels = 1;
    dsDesc.ArraySize = 1;
    dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    dsDesc.SampleDesc.Count = 1;
    if (!DX11DeviceCommand(DX11Command::CREATE_TEXTURE_2D, &dsDesc, 0, dx11.depthStencil.GetAddressOf()))
        return false;

    if (!DX11DeviceCommand(DX11Command::CREATE_DEPTH_STENCIL_VIEW, dx11.depthStencil.Get(), 0, dx11.depthStencilView.GetAddressOf()))
        return false;

    return true;
}

void dx11_DetectUWPWorkaround(const InitParam& param)
{
    DAVA::Logger::Info("[RHI-DX11] Detecting configuration ... ");

    ComPtr<IDXGIAdapter> adapter = dx11_SelectAdapter();
    if (adapter.Get())
    {
        DXGI_ADAPTER_DESC desc = {};
        adapter->GetDesc(&desc);
        char info[256] = {};
        WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, desc.Description, -1, info, countof(info) - 1, nullptr, nullptr);

        DAVA::UWPWorkaround::enableSurfaceSizeWorkaround = strstr(info, "AMD Radeon HD") && (info[strlen(info) - 1] == 'G');
        if (DAVA::UWPWorkaround::enableSurfaceSizeWorkaround)
        {
            DAVA::Logger::Warning("[RHI-DX11] Detected %s GPU, using altered configuration.", info);
            return;
        }
    }

    InitParam fullScreenParameters = param;

    const DAVA::DisplayInfo& displayInfo = DAVA::GetEngineContext()->deviceManager->GetPrimaryDisplay();
    fullScreenParameters.width = static_cast<uint32>(displayInfo.rect.dx);
    fullScreenParameters.height = static_cast<uint32>(displayInfo.rect.dy);

    DAVA::Logger::Info("[RHI-DX11] Detecting configuration by creating test device...");
    if (dx11_CreateDeviceWithAdapter(fullScreenParameters, adapter))
    {
        if (!dx11_CreateSwapChain(fullScreenParameters))
        {
            DAVA::Logger::Warning("[RHI-DX11] Failed to create SwapChain using default configuration");
            DAVA::UWPWorkaround::enableSurfaceSizeWorkaround = true;
        }
    }
    DAVA::Logger::Warning("[RHI-DX11] Using altered configuration: %s", DAVA::UWPWorkaround::enableSurfaceSizeWorkaround ? "YES" : "NO");
    dx11_DestroyDevice();
}

/*
 * Main dispatch implemenation
 */
static Api dx11_HostApi()
{
    return RHI_DX11;
}

static bool dx11_NeedRestoreResources()
{
    return false;
}

static bool dx11_TextureFormatSupported(TextureFormat format, ProgType)
{
    UINT formatSupport = 0;
    DXGI_FORMAT dxgiFormat = DX11_TextureFormat(format);

    if (dxgiFormat != DXGI_FORMAT_UNKNOWN)
        DX11DeviceCommand(DX11Command::CHECK_FORMAT_SUPPORT, dxgiFormat, &formatSupport);

    return (formatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D) != 0;
}

static void dx11_Uninitialize()
{
    QueryBufferDX11::ReleaseQueryPool();
    PerfQueryDX11::ReleasePerfQueryPool();
    dx11_DestroyDevice();
}

static void dx11_ResetBlock()
{
    if (dx11.useHardwareCommandBuffers)
    {
        ID3D11CommandList* commandList = nullptr;
        DAVA::LockGuard<DAVA::Mutex> lock(dx11.deferredContextLock);
        dx11.deferredContext->ClearState();
        DX11Check(dx11.deferredContext->FinishCommandList(FALSE, &commandList));
        DAVA::SafeRelease(commandList);
    }
    else
    {
        rhi::ConstBufferDX11::InvalidateAll();
    }

    dx11_ResizeSwapChain(resetParams.width, resetParams.height, resetParams.scaleX, resetParams.scaleY);

    if (dx11.useHardwareCommandBuffers)
    {
        DAVA::LockGuard<DAVA::Mutex> lock(dx11.deferredContextLock);
        dx11.deferredContext.Reset();
        DX11DeviceCommand(DX11Command::CREATE_DEFERRED_CONTEXT, 0, dx11.deferredContext.GetAddressOf());
    }
}

static void dx11_Reset(const ResetParam& param)
{
    {
        DAVA::LockGuard<DAVA::Mutex> lock(resetParamsSync);
        resetParams = param;
    }
    RenderLoop::SetResetPending();
}

static void dx11_SuspendRendering()
{
    FrameLoop::RejectFrames();

#if defined(__DAVAENGINE_WIN_UAP__)
    GUID guid = __uuidof(IDXGIDevice3);
    Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice3;
    if (DX11DeviceCommand(DX11Command::QUERY_INTERFACE, &guid, dxgiDevice3.GetAddressOf()))
    {
        dx11.context->ClearState();
        dxgiDevice3->Trim();
        dxgiDevice3.Reset();
    }
#endif
}

static void dx11_SynchronizeCPUGPU(uint64* cpuTimestamp, uint64* gpuTimestamp)
{
    DX11Command cmd(DX11Command::SYNC_CPU_GPU, cpuTimestamp, gpuTimestamp);
    ExecDX11(&cmd, 1);
}

static void dx11_InitContext()
{
    _DX11_RenderThreadId = GetCurrentThreadId();

#if defined(__DAVAENGINE_DEBUG__)
    dx11.hasDebugLayers = dx11_HasDebugLayers();
#endif

    dx11_DetectUWPWorkaround(dx11.initParameters);

    if (!dx11_CreateDevice(dx11.initParameters))
    {
        ReportError(dx11.initParameters, RenderingError::FailedToInitialize);
        return;
    }

    if (!dx11_CreateSwapChain(dx11.initParameters))
    {
        ReportError(dx11.initParameters, RenderingError::FailedToInitialize);
        return;
    }
    dx11_SetSwapChain(dx11.initParameters);

    if (!dx11_ResizeSwapChain(dx11.initParameters.width, dx11.initParameters.height, dx11.initParameters.scaleX, dx11.initParameters.scaleY))
    {
        ReportError(dx11.initParameters, RenderingError::FailedToInitialize);
        return;
    }

    DAVA::LockGuard<DAVA::Mutex> lock(dx11.deferredContextLock);
    DX11DeviceCommand(DX11Command::CREATE_DEFERRED_CONTEXT, 0, dx11.deferredContext.GetAddressOf());

    dx11_InitCaps();

    uint32 constBufferSize = 4 * 1024 * 1024;
    if (dx11.initParameters.shaderConstRingBufferSize)
        constBufferSize = dx11.initParameters.shaderConstRingBufferSize;

    // increasing const buffer size according to the number of frames
    // this is important on DX11, beause all shader constants for all frames are stored directly in shared ring buffer
    ConstBufferDX11::InitializeRingBuffer(constBufferSize * (1 + dx11.initParameters.threadedRenderFrameCount));
}

static bool dx11_CheckSurface()
{
    return true;
}

static bool dx11_PresentBuffer()
{
    DX11_ProcessCallResult(dx11.swapChain->Present(1, 0), __FUNCTION__, __FILE__, __LINE__);
    PerfQueryDX11::EndMeasurment(dx11.ImmediateContext());
    return true;
}

void dx11_Initialize(const InitParam& param)
{
    dx11.initParameters = param;

    VertexBufferDX11::SetupDispatch(&DispatchDX11);
    IndexBufferDX11::SetupDispatch(&DispatchDX11);
    QueryBufferDX11::SetupDispatch(&DispatchDX11);
    PerfQueryDX11::SetupDispatch(&DispatchDX11);
    TextureDX11::SetupDispatch(&DispatchDX11);
    PipelineStateDX11::SetupDispatch(&DispatchDX11);
    ConstBufferDX11::SetupDispatch(&DispatchDX11);
    DepthStencilStateDX11::SetupDispatch(&DispatchDX11);
    SamplerStateDX11::SetupDispatch(&DispatchDX11);
    RenderPassDX11::SetupDispatch(&DispatchDX11);
    SyncObjectDX11::SetupDispatch(&DispatchDX11);

    DispatchDX11.impl_Uninitialize = &dx11_Uninitialize;
    DispatchDX11.impl_Reset = &dx11_Reset;
    DispatchDX11.impl_HostApi = &dx11_HostApi;
    DispatchDX11.impl_TextureFormatSupported = &dx11_TextureFormatSupported;
    DispatchDX11.impl_NeedRestoreResources = &dx11_NeedRestoreResources;
    DispatchDX11.impl_InitContext = &dx11_InitContext;
    DispatchDX11.impl_ValidateSurface = &dx11_CheckSurface;
    DispatchDX11.impl_FinishRendering = &dx11_SuspendRendering;
    DispatchDX11.impl_ResetBlock = &dx11_ResetBlock;
    DispatchDX11.impl_SyncCPUGPU = &dx11_SynchronizeCPUGPU;
    DispatchDX11.impl_PresentBuffer = &dx11_PresentBuffer;

    SetDispatchTable(DispatchDX11);

    if (param.maxVertexBufferCount)
        VertexBufferDX11::Init(param.maxVertexBufferCount);
    if (param.maxIndexBufferCount)
        IndexBufferDX11::Init(param.maxIndexBufferCount);
    if (param.maxConstBufferCount)
        ConstBufferDX11::Init(param.maxConstBufferCount);
    if (param.maxTextureCount)
        TextureDX11::Init(param.maxTextureCount);

    if (param.maxSamplerStateCount)
        SamplerStateDX11::Init(param.maxSamplerStateCount);
    if (param.maxPipelineStateCount)
        PipelineStateDX11::Init(param.maxPipelineStateCount);
    if (param.maxDepthStencilStateCount)
        DepthStencilStateDX11::Init(param.maxDepthStencilStateCount);
    if (param.maxRenderPassCount)
        RenderPassDX11::Init(param.maxRenderPassCount);
    if (param.maxCommandBuffer)
        CommandBufferDX11::Init(param.maxCommandBuffer);

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
}
}
